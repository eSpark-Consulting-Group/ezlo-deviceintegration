#include "cored/core_service.h"
#include "cored/remote_interface/cored_remote_interface.h"
#include "cored/tests/service/test_core_service.h"
#include "database_client/dbqueryparser.h"
#include "service_interfaces/tests/fakes/fake_response_bus.h"
#include "cored/protect/protect_validator.h"
#include "cored/protect/contact_id.h"
#include "cored/protect/security_config.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/protect/protect_utils.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/protect/resources.h"
#include <gtest/gtest.h>

/*
TESTPLAN

DONE    1. Contact Id
DONE        1.1 json validator pass
DONE        1.2 json validator errors
DONE        1.3 record structure
DONE        1.4 get record method
DONE    2. Security Config
DONE        2.1 json validator pass
DONE        2.2 json validator errors
DONE        2.3 record structure
DONE        2.4 get record method
DONE        2.5 build signal field method

*/

namespace HubPlatform {
namespace UT {

using namespace Services;
using namespace CoreD::Protect;

class ProtectTestSuite : public ::testing::Test {
public:
    void SetUp() override;
    Service& getService() const;
    IDbClient& getDBClient() const;
    void cleanDB() const;

public:
    std::unique_ptr< CoreD::Tests::TestCoreServiceWithoutHouseModes > m_service;
};

Service& ProtectTestSuite::getService() const {
    return *m_service;
}

IDbClient& ProtectTestSuite::getDBClient() const {
    return getService().getDBClient();
}

void ProtectTestSuite::cleanDB() const {
    getDBClient().clean();
}

void ProtectTestSuite::SetUp() {
    m_service =
        std::make_unique< CoreD::Tests::TestCoreServiceWithoutHouseModes >( std::make_unique< Services::BusWrapper >(
            Services::BusWrapper::BusTypeTag< Services::UT::FakeResponseBus >() ) );

    getDBClient().clean();
    m_service->setUp();
}

Json securityConfig = R"({
    "_id": "0",
    "persistent": true,
    "name": "[0] Fixed Panic",
    "description": "24-hour audible fixed emergency buttons",
    "alarm": true,
    "soundType": "police",
    "entryDelay": "instant",
    "chime": "none",
    "follower": "no",
    "armedWhen": [
    "Disarm", "Stay", "Away"
    ],
    "recommendCategories": [ {"category": "", "subcategory":[ "panic"] } ],
    "signal": {
        "supervisory": {
            "send": true,
                    "event": { "code": "381", "alias": "on" },
            "restoral": { "code": "381", "alias": "off" }
        },
        "trippedArmed": {
            "send": true,
                    "event": { "code": "123", "alias": "on" },
            "restoral": { "code": "123", "alias": "off" }
        },
        "tamperedArmed": {
            "send": false
        },
        "tamperedDisarmed": {
            "send": false
        },
        "bypass": {
            "send": true,
                    "event": { "code": "570", "alias": "on" }
        }
    }
})"_json;

TEST_F( ProtectTestSuite, configJsonValidatorPass ) { /*1.1*/
    JsonObject contactId = JSON( "text"
                                 << "110 Fire Generic"
                                 << "code"
                                 << "110"
                                 << "on"
                                 << "ALERT_FIRE"
                                 << "off"
                                 << "ALERT_FIRE_OFF" );
    ProtectValidator validator( getDBClient() );
    auto answer = validator.validateAddContactIdAliasRequest( contactId );
    EXPECT_TRUE( answer.empty() );
}

TEST_F( ProtectTestSuite, configJsonValidatorFail ) { /*1.2*/
    JsonObject answer, error;
    {
        JsonObject contactId = JSON( "text"
                                     << "110 Fire Generic"
                                     << "code" << 110 << "on"
                                     << "ALERT_FIRE"
                                     << "off"
                                     << "ALERT_FIRE_OFF" );
        ProtectValidator validator( getDBClient() );
        answer = validator.validateAddContactIdAliasRequest( contactId );
        EXPECT_FALSE( answer.empty() );
        json_get_obj( answer, "error", error );

        EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.invalid" );
        EXPECT_EQ( json_get_str( error, "field" ), "code" );
        EXPECT_EQ( json_get_str( error, "message" ), "Wrong params" );
    }

    {
        JsonObject contactId = JSON( "text"
                                     << "110 Fire Generic"
                                     << "code"
                                     << "110"
                                     << "off"
                                     << "ALERT_FIRE_OFF" );
        ProtectValidator validator( getDBClient() );
        answer = validator.validateAddContactIdAliasRequest( contactId );
        EXPECT_FALSE( answer.empty() );
        json_get_obj( answer, "error", error );

        EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.wrong_field" );
        EXPECT_EQ( json_get_str( error, "field" ), "on" );
        EXPECT_EQ( json_get_str( error, "message" ), "Wrong field of object" );
    }

    {
        JsonObject contactId =
            JSON( "text"
                  << "110FireGeneric110FireGeneric110FireGeneric110FireGeneric110FireGeneric110FireGeneric110FireGeneri"
                     "c110FireGeneric110FireGeneric110FireGeneric110FireGeneric110FireGeneric110FireGeneric110FireGener"
                     "ic110FireGeneric110FireGeneric110FireGeneric"
                  << "code"
                  << "110"
                  << "on"
                  << "ALERT_FIRE"
                  << "off"
                  << "ALERT_FIRE_OFF" );
        ProtectValidator validator( getDBClient() );
        answer = validator.validateAddContactIdAliasRequest( contactId );
        EXPECT_FALSE( answer.empty() );
        json_get_obj( answer, "error", error );

        EXPECT_EQ( json_get_str( error, "data" ), "ezlo.value.too.long" );
        EXPECT_EQ( json_get_str( error, "field" ), "text" );
        EXPECT_EQ( json_get_str( error, "message" ), "Value is too long" );
    }
}

TEST_F( ProtectTestSuite, recordJsonDBStructure ) { /*1.3*/
    JsonObject contactId = JSON( "text"
                                 << "110 Fire Generic"
                                 << "code"
                                 << "110"
                                 << "on"
                                 << "ALERT_FIRE"
                                 << "off"
                                 << "ALERT_FIRE_OFF" );

    Json expectedDBRecord = JSON( "_id"
                                  << "110"
                                  << "off"
                                  << "ALERT_FIRE_OFF"
                                  << "on"
                                  << "ALERT_FIRE"
                                  << "persistent" << false << "text"
                                  << "110 Fire Generic" );
    ContactIdInfo contactIdInfo;
    contactIdInfo.fromJson( contactId );
    auto data = contactIdInfo.toJson();
    getDBClient().insert( CoreD::Model::Resources::DBNames::ContactIdsDB, data );

    auto id = json_get_str( data, db::ID_FIELD_NAME );
    EXPECT_FALSE( id.empty() );
    auto contactIdJson = getDBClient().findOne(
        CoreD::Model::Resources::DBNames::ContactIdsDB, JSON( db::ID_FIELD_NAME << id ), nullptr );

    EXPECT_EQ( contactIdJson, expectedDBRecord );
}

TEST_F( ProtectTestSuite, getContactIdTest ) { /*1.4*/
    JsonObject contactId = JSON( "text"
                                 << "110 Fire Generic"
                                 << "code"
                                 << "110"
                                 << "on"
                                 << "ALERT_FIRE"
                                 << "off"
                                 << "ALERT_FIRE_OFF" );

    Json expectedDBRecord = JSON( "_id"
                                  << "110"
                                  << "off"
                                  << "ALERT_FIRE_OFF"
                                  << "on"
                                  << "ALERT_FIRE"
                                  << "persistent" << false << "text"
                                  << "110 Fire Generic" );
    ContactIdInfo contactIdInfo;
    contactIdInfo.fromJson( contactId );
    auto data = contactIdInfo.toJson();
    getDBClient().insert( CoreD::Model::Resources::DBNames::ContactIdsDB, data );

    auto id = json_get_str( data, db::ID_FIELD_NAME );
    EXPECT_FALSE( id.empty() );
    auto contactIdJson = CoreD::Protect::ProtectUtils::getContactIdObject( getDBClient(), id );
    EXPECT_EQ( contactIdJson, expectedDBRecord );
}

TEST_F( ProtectTestSuite, securityConfigJsonValidatorPass ) { /*2.1*/
    ProtectValidator validator( getDBClient() );
    auto answer = validator.validateAddSecurityConfigRequest( securityConfig );
    EXPECT_TRUE( answer.empty() );
}

TEST_F( ProtectTestSuite, securityConfigJsonValidatorFail ) { /*2.2*/
    JsonObject answer, error;
    {
        auto config = securityConfig;
        config[ "alarm" ] = "true";
        ProtectValidator validator( getDBClient() );
        answer = validator.validateAddSecurityConfigRequest( config );
        EXPECT_FALSE( answer.empty() );
        json_get_obj( answer, "error", error );

        EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.invalid" );
        EXPECT_EQ( json_get_str( error, "field" ), "alarm" );
        EXPECT_EQ( json_get_str( error, "message" ), "Wrong params" );
    }

    {
        auto config = securityConfig;
        config[ "signal" ] = "test";
        ProtectValidator validator( getDBClient() );
        answer = validator.validateAddSecurityConfigRequest( config );
        EXPECT_FALSE( answer.empty() );
        json_get_obj( answer, "error", error );

        EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.invalid" );
        EXPECT_EQ( json_get_str( error, "field" ), "signal" );
        EXPECT_EQ( json_get_str( error, "message" ), "Wrong params" );
    }

    {
        auto config = securityConfig;
        config[ "description" ] = "testtesttesttesttesttesttesttesttesttesttesttesttesttesttest122212121212121212121222"
                                  "1212121212121212121212121212121212121212121212122121";
        ProtectValidator validator( getDBClient() );
        answer = validator.validateAddSecurityConfigRequest( config );
        EXPECT_FALSE( answer.empty() );
        json_get_obj( answer, "error", error );

        EXPECT_EQ( json_get_str( error, "data" ), "ezlo.value.too.long" );
        EXPECT_EQ( json_get_str( error, "field" ), "description" );
        EXPECT_EQ( json_get_str( error, "message" ), "Value is too long" );
    }
}

TEST_F( ProtectTestSuite, securityConfigJsonDBStructure ) { /*2.3*/

    SecurityConfigInfo securityConfigInfo;
    securityConfigInfo.fromJson( securityConfig );
    auto data = securityConfigInfo.toJson();
    getDBClient().insert( CoreD::Model::Resources::DBNames::SecurityConfigsDB, data );

    auto id = json_get_str( data, db::ID_FIELD_NAME );
    EXPECT_FALSE( id.empty() );
    auto securityConfigJson = getDBClient().findOne(
        CoreD::Model::Resources::DBNames::SecurityConfigsDB, JSON( db::ID_FIELD_NAME << id ), nullptr );

    EXPECT_EQ( securityConfigJson, securityConfig );
}

TEST_F( ProtectTestSuite, getSecurityConfigTest ) { /*2.4*/

    SecurityConfigInfo securityConfigInfo;
    securityConfigInfo.fromJson( securityConfig );
    auto data = securityConfigInfo.toJson();
    getDBClient().insert( CoreD::Model::Resources::DBNames::SecurityConfigsDB, data );

    auto id = json_get_str( data, db::ID_FIELD_NAME );
    EXPECT_FALSE( id.empty() );
    auto securityConfigJson = CoreD::Protect::ProtectUtils::getSecurityConfigObject( getDBClient(), id );

    EXPECT_EQ( securityConfigJson, securityConfig );
}

TEST_F( ProtectTestSuite, buildSignalFieldTest ) { /*2.5*/
    using namespace CoreD;

    const JsonObject contactId = R"(
    {
        "_id": "123",
        "text": "123 Panic Audible",
        "on": "ALERT_PANIC_AUDIBLE",
        "off": "ALERT_PANIC_AUDIBLE_OFF"
    }
    )"_json;

    getDBClient().insert( Model::Resources::DBNames::FeaturesDB,
                          JSON( "_id"
                                << "protect"
                                << "status" << true ) );
    getDBClient().insert( Model::Resources::DBNames::ContactIdsDB, contactId );

    SecurityConfigInfo securityConfigInfo;
    securityConfigInfo.fromJson( securityConfig );
    JsonObject result;
    securityConfigInfo.buildSignalField(
        securityConfigInfo.getSignalObject(), Protect::Resources::SignalFields::TrippedArmed, getDBClient(), result );

    ASSERT_TRUE( json_contains( result, "signal" ) );
    EXPECT_EQ( json_get_str( result, "signal" ), "ALERT_PANIC_AUDIBLE" );
}

}  // namespace UT
}  // namespace HubPlatform
