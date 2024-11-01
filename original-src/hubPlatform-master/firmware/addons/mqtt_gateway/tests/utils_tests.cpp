#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "mqtt_gateway/utils/utils.h"
#include "mqtt_gateway/resources/resources.h"

#include "database_client/fake_db/fake_db_client.h"
#include "database_client/dbqueryparser.h"
#include "json_utils/json_utils.h"
#include "common_utils/cpp14.h"

/*

TESTPLAN

DONE    1. Database utils tests
DONE        1.1 Add client to DB
DONE        1.2 Read clients from DB
DONE        1.3 Remove client from DB

DONE    2. RPC utils tests
DONE        2.1 Build MQTT message response
DONE        2.2 Build MQTT error message response
DONE        2.3 Build error message response

*/

namespace HubPlatform {
namespace UT {

using namespace MQTTGateway;

class MQTTGatewayUtilsDatabaseTestSuite : public ::testing::Test {
public:
    void SetUp() override {
        m_dbClientPtr = std::make_unique< HubPlatform::UT::FakeDbClient >();
    }

    IDbClient& getDBClient() {
        return *m_dbClientPtr;
    }

protected:
    std::unique_ptr< IDbClient > m_dbClientPtr;
};

TEST_F( MQTTGatewayUtilsDatabaseTestSuite, AddClientToDb ) /* 1.1 */ {
    using namespace MQTTGateway::Resources::DB;
    auto& dbClient = getDBClient();

    const std::string clientA = "clientA";
    const std::string clientB = "clientB";

    {
        Utils::addClientToDB( dbClient, clientA );
        auto results = dbClient.findAll( TableNames::RegisteredClientsDBName, JSON_EMPTY_OBJ, nullptr );
        ASSERT_EQ( results.size(), 1 );
        EXPECT_EQ( json_get_str( results.front(), db::ID_FIELD_NAME ), clientA );
    }

    {  // Add duplicate
        Utils::addClientToDB( dbClient, clientA );
        auto results = dbClient.findAll( TableNames::RegisteredClientsDBName, JSON_EMPTY_OBJ, nullptr );
        ASSERT_EQ( results.size(), 1 );
        EXPECT_EQ( json_get_str( results.front(), db::ID_FIELD_NAME ), clientA );
    }

    {
        Utils::addClientToDB( dbClient, clientB );
        auto results = dbClient.findAll( TableNames::RegisteredClientsDBName, JSON_EMPTY_OBJ, nullptr );
        ASSERT_EQ( results.size(), 2 );

        std::set< std::string > ids;
        for( auto& record : results )
            ids.emplace( json_get_str( record, db::ID_FIELD_NAME ) );

        EXPECT_EQ( ids, std::set< std::string >( { clientA, clientB } ) );
    }
};

TEST_F( MQTTGatewayUtilsDatabaseTestSuite, ReadClientsFromDb ) /* 1.2 */ {
    using namespace MQTTGateway::Resources::DB;
    using Clients = std::set< std::string >;
    auto& dbClient = getDBClient();

    const std::string clientA = "clientA";
    const std::string clientB = "clientB";

    EXPECT_EQ( Utils::readClientsFromDB( dbClient ), Clients( {} ) );

    Utils::addClientToDB( dbClient, clientA );
    EXPECT_EQ( Utils::readClientsFromDB( dbClient ), Clients( { clientA } ) );

    Utils::addClientToDB( dbClient, clientB );
    EXPECT_EQ( Utils::readClientsFromDB( dbClient ), Clients( { clientA, clientB } ) );
};

TEST_F( MQTTGatewayUtilsDatabaseTestSuite, RemoveClientFromDb ) /* 1.3 */ {
    using namespace MQTTGateway::Resources::DB;
    auto& dbClient = getDBClient();

    const std::string clientA = "clientA";
    const std::string clientB = "clientB";

    {  // Remove from empty DB
        EXPECT_NO_THROW( Utils::removeClientFromDB( dbClient, clientA ) );
        EXPECT_EQ( dbClient.findAll( TableNames::RegisteredClientsDBName, JSON_EMPTY_OBJ, nullptr ).size(), 0 );
    }

    Utils::addClientToDB( dbClient, clientA );
    Utils::addClientToDB( dbClient, clientB );

    {  // Remove unknown key DB
        EXPECT_NO_THROW( Utils::removeClientFromDB( dbClient, "unknown" ) );
        EXPECT_EQ( dbClient.findAll( TableNames::RegisteredClientsDBName, JSON_EMPTY_OBJ, nullptr ).size(), 2 );
    }

    {  // Remove first client
        Utils::removeClientFromDB( dbClient, clientA );
        auto results = dbClient.findAll( TableNames::RegisteredClientsDBName, JSON_EMPTY_OBJ, nullptr );
        ASSERT_EQ( results.size(), 1 );
        EXPECT_EQ( json_get_str( results.front(), db::ID_FIELD_NAME ), clientB );
    }

    {  // Remove last client
        Utils::removeClientFromDB( dbClient, clientB );
        auto results = dbClient.findAll( TableNames::RegisteredClientsDBName, JSON_EMPTY_OBJ, nullptr );
        ASSERT_EQ( results.size(), 0 );
    }
};

class MQTTGatewayUtilsTestSuite : public ::testing::Test {};

TEST_F( MQTTGatewayUtilsTestSuite, BuildMQTTResponse ) /* 2.1 */ {
    const auto expectedResponse = R"(
    {
        "msg_group": "group",
        "msg_class": "response",
        "msg_subclass": "ResponseResult",
        "result": {
            "responseBody": {
                "key": "value"
            },
            "uuid": "1234"
        }
    }
    )"_json;

    auto response = Utils::buildMQTTResponse( "group",
                                              "1234",
                                              JSON( "key"
                                                    << "value" ) );

    EXPECT_EQ( response, expectedResponse );
};

TEST_F( MQTTGatewayUtilsTestSuite, BuildMQTTErrorResponse ) /* 2.2 */ {
    const auto expectedResponse = R"(
    {
        "msg_group": "group",
        "msg_class": "response",
        "msg_subclass": "ResponseResult",
        "result": {
            "responseBody": {
                "error": {
                    "code": -32500,
                    "data": "mqtt.client.not.registered",
                    "message": "MQTT client not registered",
                    "reason": "Not registered"
                }
            },
            "uuid": "1234"
        }
    }
    )"_json;

    auto response =
        Utils::buildMQTTErrorResponse( "group", "1234", Errors::Error::MQTT_CLIENT_NOT_REGISTERED, "Not registered" );
    EXPECT_EQ( response, expectedResponse );
};

TEST_F( MQTTGatewayUtilsTestSuite, BuildErrorResponseBody ) /* 2.3 */ {
    const auto expectedResponse = R"(
    {
        "error": {
            "code": -32500,
            "data": "mqtt.client.not.registered",
            "message": "MQTT client not registered",
            "reason": "Not registered"
        }
    }
    )"_json;

    auto response = Utils::buildErrorResponseBody( Errors::Error::MQTT_CLIENT_NOT_REGISTERED, "Not registered" );
    EXPECT_EQ( response, expectedResponse );
};

}  // namespace UT
}  // namespace HubPlatform
