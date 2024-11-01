#include "common_test_resources/resources/uid_resources.h"
#include "cored/model/values/value_factory.h"
#include "common_test_resources/fixtures/core_test_fixture.h"
#include <gtest/gtest.h>

namespace HubPlatform {
namespace Tests {

class PolicyTests : public CoreNmaTest {
public:
    void SetUp() override;
    void TearDown() override;

    static void checkError( const Json& answer, int errorCode, const std::string& data, const std::string& message );
    static void checkNoError( const JsonObject& answer, const std::string& message );
    static void checkList( const Json& answer, const std::string& message );
    static std::string makeCommand( std::vector< std::string > ids );

protected:
    std::string m_policyId;
};

void PolicyTests::SetUp() {
    CoreNmaTest::SetUp();
    callingFile( __FILE__ );
    std::string name = "testPolicy1";
    std::string protectConfig = "1";
    std::string deviceGroup = "2";
    const std::vector< std::string >& meshbots = { "meshbot1", "meshbot2" };
    bool persistent = false;
    m_policyId = addPolicy( makeRemote(), name, protectConfig, deviceGroup, meshbots, persistent, __LINE__ );
}

void PolicyTests::TearDown() {
    removePolicy( makeRemote(), m_policyId, __LINE__ );
    CoreNmaTest::TearDown();
}

void PolicyTests::checkError( const Json& answer, int errorCode, const std::string& data, const std::string& message ) {
    Json error;
    LOGGER_TRACE( cc::j( answer ) );
    EXPECT_FALSE( answer.empty() ) << message;
    EXPECT_TRUE( json_get( answer, "error", error ) ) << message;
    EXPECT_TRUE( error.is_object() ) << message;

    EXPECT_EQ( json_get_int( error, "code", -1 ), errorCode ) << message;
    if( !data.empty() )
        EXPECT_EQ( error[ "data" ], data ) << message;
}

void PolicyTests::checkNoError( const JsonObject& answer, const std::string& message ) {
    Json error;
    EXPECT_FALSE( answer.empty() ) << message;
    EXPECT_TRUE( json_get( answer, "error", error ) ) << message;
    EXPECT_TRUE( error.is_null() ) << message;
}

void PolicyTests::checkList( const Json& answer, const std::string& message ) {
    Json listing;
    EXPECT_TRUE( json_get( answer, { "result", "policies" }, listing ) ) << message;
    EXPECT_TRUE( listing.is_array() ) << message;
    EXPECT_GE( listing.size(), 1 ) << message;
}

namespace Resources {
std::string hubPolicyList = R"({
        "method": "hub.policies.list",
        "id": "_ID_",
        "params": {
            __OPTIONAL_IDS__
        }
    })";
std::string hubPolicyGet = R"({
        "method": "hub.policy.get",
        "id": "_ID_",
        "params": {
            __OPTIONAL_IDS__
        }
    })";
const char optionalIdsPattern[] = "__OPTIONAL_IDS__";
}  // namespace Resources

TEST_F( PolicyTests, getPoliciesList ) {
    {
        Json request = replaceTokens( Resources::hubPolicyList, { { Resources::optionalIdsPattern, "" } } );
        auto answer = nma().sendtoHubAndGetAnswer( request );
        checkNoError( answer, NMA_TEST_MAKE_MESSAGE( "get whole list" ) );
        checkList( answer, NMA_TEST_MAKE_MESSAGE( "get whole list" ) );
    }
    {
        Json request =
            replaceTokens( Resources::hubPolicyList, { { Resources::optionalIdsPattern, R"("policyIds": [])" } } );
        auto answer = nma().sendtoHubAndGetAnswer( request );
        checkNoError( answer, NMA_TEST_MAKE_MESSAGE( "get whole list" ) );
        checkList( answer, NMA_TEST_MAKE_MESSAGE( "get whole list" ) );
    }
}

TEST_F( PolicyTests, getPoliciesListById ) {
    {
        Json request = replaceTokens( Resources::hubPolicyList,
                                      { { Resources::optionalIdsPattern, "\"policyIds\": [\"NoSuchID\"]" } } );
        auto answer = nma().sendtoHubAndGetAnswer( request );
        Json list;
        EXPECT_TRUE( json_get( answer, { "result", "policies" }, list ) );
        EXPECT_EQ( list.size(), 0 );
    }
    {
        Json request =
            replaceTokens( Resources::hubPolicyList,
                           { { Resources::optionalIdsPattern, "\"policyIds\": [\"" + m_policyId + "\"]" } } );
        auto answer = nma().sendtoHubAndGetAnswer( request );
        Json list;
        EXPECT_TRUE( json_get( answer, { "result", "policies" }, list ) );
        EXPECT_EQ( list.size(), 1 );
    }
}

TEST_F( PolicyTests, getPoliciesListErrors ) {
    {
        Json request = replaceTokens( Resources::hubPolicyList,
                                      { { Resources::optionalIdsPattern, R"("policyIds": "some string")" } } );
        auto answer = nma().sendtoHubAndGetAnswer( request );
        checkError( answer, -32602, "rpc.params.invalid", NMA_TEST_MAKE_MESSAGE( "not an array" ) );
    }
}

TEST_F( PolicyTests, getPolicyGet ) {
    {
        Json request = replaceTokens( Resources::hubPolicyGet,
                                      { { Resources::optionalIdsPattern, "\"id\": \"" + m_policyId + "\"" } } );
        auto answer = nma().sendtoHubAndGetAnswer( request );
        checkNoError( answer, NMA_TEST_MAKE_MESSAGE( "get policy" ) );
    }
}

TEST_F( PolicyTests, getPolicyGetErrors ) {
    {
        Json request =
            replaceTokens( Resources::hubPolicyGet, { { Resources::optionalIdsPattern, "\"id\": \"BadIDToTest\"" } } );
        auto answer = nma().sendtoHubAndGetAnswer( request );
        checkError( answer, -32602, "ezlo.not.exist", NMA_TEST_MAKE_MESSAGE( "does not exist" ) );
    }
    {
        Json request = replaceTokens( Resources::hubPolicyGet,
                                      { { Resources::optionalIdsPattern, "\"id\": [\"" + m_policyId + "\"]" } } );
        auto answer = nma().sendtoHubAndGetAnswer( request );
        checkError( answer, -32603, "internal.operation.error", NMA_TEST_MAKE_MESSAGE( "not a string" ) );
    }
}

}  // namespace Tests
}  // namespace HubPlatform
