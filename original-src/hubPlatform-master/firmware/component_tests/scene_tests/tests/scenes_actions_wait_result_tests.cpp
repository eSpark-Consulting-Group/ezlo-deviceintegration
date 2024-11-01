#include "fixtures/scenes_expressions_fixture.h"
#include "common_test_resources/resources/scene_manager_resources.h"
#include "cored/remote_interface/cored_interface.h"
#include <gtest/gtest.h>
#include "test_utils/component_tests/http_server/http_server.h"
#include "system_utils/process/shared_mutex.h"
#include "system_utils/find_free_local_port.h"
#include <curl/curl.h>
#include <tuple>

/*

TESTPLAN

DONE        1. partiallyFinishedForTwoDifferentActions Create two scene with 'wait_result' exec_policy and
               run failed sendHttp and setItemValue actions, check finish result and order
DONE        2. finishTwoActions run two sendHttp actions with wrong url
DONE        3. failTwoActions run two sendHttp actions with correct url
DONE        4. partiallyFinishedForTwoDifferentActionsWaitResultForFirst run scene with default exec_policy and two
               actions, and first from them has  'wait_result' policy
DONE        5. twoActionsCheckAndWait: a few combinations of actions with check and wait policy
DONE        6. threeActionsIgnoreCheckAndWait: a few combinations of actions with check ignore and wait policy

*/

namespace HubPlatform {
namespace Tests {

const char* const WaitResultPolicy = "wait_result";
const char* const CheckResultPolicy = "check_result";
const char* const IgnoreResultPolicy = "ignore_result";

const char* const CorrectUrl = "/reflect";
const char* const IncorrectUrl = "/fail_url";

class ScenesActionsWaitResultTest : public ScenesExpressionsFixture {
public:
    ScenesActionsWaitResultTest();
    void SetUp() override;
    void TearDown() override;
    static void SetUpTestCase();

    std::string createSendHttpGlobalVariableAction( const std::string& url,
                                                    const std::string& expressionName,
                                                    const std::string& execPolicy,
                                                    int codeLine );
    std::string createSetItemValueAction( const std::string& itemId, int value, const std::string& execPolicy = "" );
    std::string createSendHttpAction( const std::string& url, const std::string& execPolicy = "" );

    static void startOnUniquePorts( Tests::HttpServer& server );
    static HttpServer& getHttpServer();
    std::tuple< std::string, JsonObject > createAndRunScene( std::vector< std::string > actions,
                                                             const std::string& execPolicy,
                                                             int codeLine );

protected:
    CoreDRemoteInterface::CoreDInterface& m_remote;
};

ScenesActionsWaitResultTest::ScenesActionsWaitResultTest()
: m_remote( makeRemote( nma() ) ) {}

void ScenesActionsWaitResultTest::SetUp() {
    ScenesExpressionsFixture::SetUp();
    clearExistingDevices( __LINE__ );
    callingFile( __FILE__ );
    getPort() = getHttpServer().getPort();
}

void ScenesActionsWaitResultTest::TearDown() {
    ignoreBroadcasts(
        []( const Json& broadcast ) {
            Log::error( cc::error( "# TearDown: Not handled broadcasts: " ) + cc::j( broadcast ) );
            return true;
        },
        __FILE__,
        __LINE__,
        std::chrono::milliseconds( 10 ) );

    Json answer, expressions;
    runExpressionsList( answer, false );
    checkExpressions( answer, expressions, __LINE__ );

    if( expressions.size() > 0 ) {
        LOGGER_ERROR( "# TearDown: Expressions list should be empty, but has elements:  " +
                      std::to_string( expressions.size() ) );
        ASSERT_EQ( expressions.size(), 0 ) << "Expressions list should be empty";
    }

    ScenesExpressionsFixture::TearDown();
}

void ScenesActionsWaitResultTest::SetUpTestCase() {
    curl_global_init( CURL_GLOBAL_ALL );
    startOnUniquePorts( getHttpServer() );
}

void ScenesActionsWaitResultTest::startOnUniquePorts( HttpServer& server ) {
    Process::SharedMutex processMutex( "uniquePortMutex" );
    std::unique_lock< Process::SharedMutex > lock( processMutex );
    int insecurePort = system_utils::getUniqueFreeLocalPort();
    server.setPort( insecurePort );
    int securePort = system_utils::getUniqueFreeLocalPort();

    server.setSecurePort( securePort );
    server.start();
    lock.unlock();
}

HttpServer& ScenesActionsWaitResultTest::getHttpServer() {
    static HttpServer lws;
    return lws;
}

std::string ScenesActionsWaitResultTest::createSendHttpGlobalVariableAction( const std::string& url,
                                                                             const std::string& expressionName,
                                                                             const std::string& execPolicy,
                                                                             int codeLine ) {
    bool isExpectedBroadcast = true;
    scenesVariableExpressionSetType< int >(
        expressionName, 1, "hub.expression.added", "int", {}, isExpectedBroadcast, codeLine );

    return replaceTokensInString( sendHttpRequestSaveResultThenBlock,
                                  { { "__URL__", makeUrl( url.c_str() ) },
                                    { "__USER__", "" },
                                    { "__PASSWORD__", "" },
                                    { "__REQUEST_TYPE__", "GET" },
                                    { "__CONTENT_TYPE__", "text/plain" },
                                    { "__CONTENT__", "" },
                                    { "__HEADERS__", "" },
                                    { "__SKIP_SECURITY__", "true" },
                                    { "__GLOBAL_VARIABLE_NAME__", expressionName },
                                    { "__EXEC_POLICY__", execPolicy.empty() ? "" : execPolicy } } );
}

std::string ScenesActionsWaitResultTest::createSetItemValueAction( const std::string& itemId,
                                                                   int value,
                                                                   const std::string& execPolicy ) {
    CoreDRemoteInterface::CoreDInterface& coredRemote = makeRemote();
    coredRemote.setItemValue( JSON( CoreD::Model::Resources::ItemFields::IdFieldName
                                    << itemId << CoreD::Model::Resources::ItemFields::ValueFieldName << 1 ) );
    checkItemUpdateBroadcast( itemId, 1, __LINE__ );

    return replaceTokensInString( setItemValueThenBlock,
                                  { { "__ITEM_ID__", itemId },
                                    { "__TYPE__", "int" },
                                    { "__EXEC_POLICY__", execPolicy.empty() ? IgnoreResultPolicy : execPolicy },
                                    { "__VALUE__", std::to_string( value ) } } );
}

std::string ScenesActionsWaitResultTest::createSendHttpAction( const std::string& url, const std::string& execPolicy ) {
    return replaceTokensInString( sendHttpRequestThenBlock,
                                  { { "__URL__", makeUrl( url.c_str() ) },
                                    { "__USER__", "" },
                                    { "__PASSWORD__", "" },
                                    { "__REQUEST_TYPE__", "GET" },
                                    { "__CONTENT_TYPE__", "text/plain" },
                                    { "__CONTENT__", "" },
                                    { "__HEADERS__", "" },
                                    { "__SKIP_SECURITY__", "true" },
                                    { "__EXEC_POLICY__", execPolicy.empty() ? "" : execPolicy } } );
}

std::tuple< std::string, JsonObject > ScenesActionsWaitResultTest::createAndRunScene(
    std::vector< std::string > actions,
    const std::string& execPolicy,
    int codeLine ) {
    std::string actionString;
    for( auto action : actions ) {
        actionString += action;
        actionString += ",";
    }

    if( actions.size() > 1 ) {
        actionString.pop_back();
    }

    auto request = replaceTokens( createSceneRequest,
                                  { { "__NAME__", "WaitResult" },
                                    { "__WHEN_BLOCKS__", "" },
                                    { "__EXEC_POLICY__", execPolicy },
                                    { "__THEN_BLOCKS__", actionString } } );

    SceneID sceneId = createScene( request, __LINE__ );
    runSceneManually( sceneId, __LINE__ );
    return std::make_tuple( sceneId, request );
}

TEST_F( ScenesActionsWaitResultTest, partiallyFinishedForTwoDifferentActions ) /* 1 */ {
    const std::string expressionName = "setHttpGlobalVariableAction";
    std::string actionBlockHttpGlobalVariableAction = createSendHttpGlobalVariableAction( "/fail_url?__delayed=1",
                                                                                          expressionName,
                                                                                          ""
                                                                                          "",
                                                                                          __LINE__ );
    auto device = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );
    auto setItemValueBlock = createSetItemValueAction( device.m_itemId, 10 );

    SceneID sceneId;
    JsonObject request;
    std::tie( sceneId, request ) =
        createAndRunScene( { actionBlockHttpGlobalVariableAction, setItemValueBlock }, WaitResultPolicy, __LINE__ );

    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    checkBroadcast( "", "hub.expression.changed", { { "name", expressionName } }, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "partially_finished", __LINE__ );

    auto broadcast = waitForBroadcast( "hub.item.updated", NMA_TEST_MAKE_MESSAGE( "no broadcast" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.updated" );

    deleteSceneExpression( expressionName, __LINE__ );
    deleteScene( sceneId, __LINE__ );
}

TEST_F( ScenesActionsWaitResultTest, finishedForTwoActions ) /* 2 */ {
    std::string action = createSendHttpAction( CorrectUrl );

    SceneID sceneId;
    JsonObject request;
    std::tie( sceneId, request ) = createAndRunScene( { action, action }, WaitResultPolicy, __LINE__ );

    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    deleteScene( sceneId, __LINE__ );
}

TEST_F( ScenesActionsWaitResultTest, failForTwoActions ) /* 3 */ {
    std::string action = createSendHttpAction( IncorrectUrl );
    SceneID sceneId;
    JsonObject request;
    std::tie( sceneId, request ) = createAndRunScene( { action, action }, WaitResultPolicy, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "failed", __LINE__ );

    deleteScene( sceneId, __LINE__ );
}

TEST_F( ScenesActionsWaitResultTest, partiallyFinishedForTwoDifferentActionsWaitResultForFirst ) /* 4 */ {
    const std::string expressionName = "setHttpGlobalVariableAction";
    std::string actionBlockHttpGlobalVariableAction =
        createSendHttpGlobalVariableAction( IncorrectUrl, expressionName, WaitResultPolicy, __LINE__ );
    auto device = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );
    auto setItemValueBlock = createSetItemValueAction( device.m_itemId, 10 );

    SceneID sceneId;
    JsonObject request;
    std::tie( sceneId, request ) =
        createAndRunScene( { actionBlockHttpGlobalVariableAction, setItemValueBlock }, IgnoreResultPolicy, __LINE__ );

    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    checkBroadcast( "", "hub.expression.changed", { { "name", expressionName } }, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "partially_finished", __LINE__ );

    auto broadcast = waitForBroadcast( "hub.item.updated", NMA_TEST_MAKE_MESSAGE( "no broadcast" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.updated" );

    deleteSceneExpression( expressionName, __LINE__ );
    deleteScene( sceneId, __LINE__ );
}

TEST_F( ScenesActionsWaitResultTest, twoActionsCheckAndWait ) /* 5 */ {
    // wait and then check
    std::string action_wait = createSendHttpAction( IncorrectUrl, WaitResultPolicy );
    std::string action_check = createSendHttpAction( CorrectUrl, CheckResultPolicy );
    SceneID sceneId;
    JsonObject request;
    std::tie( sceneId, request ) = createAndRunScene( { action_check, action_wait }, IgnoreResultPolicy, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "partially_finished", __LINE__ );

    deleteScene( sceneId, __LINE__ );

    // wait and then check
    action_check = createSendHttpAction( IncorrectUrl, CheckResultPolicy );
    action_wait = createSendHttpAction( CorrectUrl, WaitResultPolicy );

    std::tie( sceneId, request ) = createAndRunScene( { action_check, action_wait }, IgnoreResultPolicy, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "failed", __LINE__ );

    deleteScene( sceneId, __LINE__ );
}

TEST_F( ScenesActionsWaitResultTest, threeActionsIgnoreCheckAndWait ) /* 6 */ {
    // wait and then check
    std::string action_ignore = createSendHttpAction( IncorrectUrl, IgnoreResultPolicy );
    std::string action_wait = createSendHttpAction( IncorrectUrl, WaitResultPolicy );
    std::string action_check = createSendHttpAction( CorrectUrl, CheckResultPolicy );
    SceneID sceneId;
    JsonObject request;
    std::tie( sceneId, request ) =
        createAndRunScene( { action_ignore, action_wait, action_check }, IgnoreResultPolicy, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "partially_finished", __LINE__ );

    deleteScene( sceneId, __LINE__ );

    std::tie( sceneId, request ) =
        createAndRunScene( { action_wait, action_ignore, action_check }, IgnoreResultPolicy, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "partially_finished", __LINE__ );

    deleteScene( sceneId, __LINE__ );

    std::tie( sceneId, request ) =
        createAndRunScene( { action_check, action_wait, action_ignore }, IgnoreResultPolicy, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "partially_finished", __LINE__ );

    deleteScene( sceneId, __LINE__ );
}

}  // namespace Tests
}  // namespace HubPlatform
