#include "common_test_resources/resources/scene_manager_resources.h"
#include "common_test_resources/resources/cored_resources.h"
#include "fixtures/scenes_expressions_fixture.h"
#include "test_utils/component_tests/http_server/http_server.h"
#include "system_utils/process/shared_mutex.h"
#include "system_utils/find_free_local_port.h"
#include "http/http_options.h"
#include "http/http.h"
#include "database_client/dbqueryparser.h"
#include "database_client/idbclient.h"
#include "json_utils/json_utils.h"
#include "common_test_resources/fixtures/scene_manager_tests_fixture.h"
#include "cored/remote_interface/cored_remote_interface.h"
#include <gtest/gtest.h>

#include <curl/curl.h>
#include "tiny/tiny_consolecolors.h"

namespace HubPlatform {
namespace Tests {

/*

TESTPLAN

DONE    1.  hub scenes stop method test
DONE        1.1  hub scenes stop success
DONE        1.2  hub scenes stop error
DONE    2.  test execution context
DONE        2.1  check execution context test
DONE    3.  stop sync/async actions
DONE        3.1  stop scene with 'check_result' exec_policy actions
DONE        3.2  stop scene with 'wait_result' exec_policy actions
DONE        3.3  stop scene with 'ignore_result' exec_policy actions
DONE        3.4  stop sync actions
DONE        3.5  stop async actions
DONE        3.6  stop actions with another block type
DONE    4.  stop delayed actions
DONE        4.1  stop delayed actions with 'check_result' exec_policy
DONE        4.2  stop delayed actions with 'ignore_result' exec_policy
DONE        4.3  stop delayed actions with 'wait_result' exec_policy
DONE    5.  isSceneState event
DONE        5.1  isSceneState event check

*/

class SceneStopMeshbotTests : public ScenesExpressionsFixture {
public:
    void SetUp() override;
    void TearDown() override;
    static void SetUpTestCase();

    static void startOnUniquePorts( Tests::HttpServer& server );
    static HttpServer& getHttpServer();
    void sendStop( const std::string& sceneId, const std::string& type, int codeLine );

    std::string createActionRequestWithExecPolicy( std::string execPolicy );
    JsonObject sceneRequestWithFewActions( const std::string& actionBlock );
    std::string createDelayedActionRequestWithExecPolicy( const std::string& itemId, std::string execPolicy );
    void stopDelayedActionsWithExecPolicy( std::string execPolicy, int codeLine );
    void checkRunContextRecords( int codeLine );
};

void SceneStopMeshbotTests::SetUp() {
    ScenesExpressionsFixture::SetUp();
    clearExistingDevices( __LINE__ );
    callingFile( __FILE__ );
    getPort() = getHttpServer().getPort();
    nma().getDBClient().clear( HaTempDBSceneExecutionContextDBName );
}

void SceneStopMeshbotTests::TearDown() {
    ignoreBroadcasts(
        []( const Json& broadcast ) {
            Log::error( cc::error( "# TearDown: Not handled broadcasts: " ) + cc::j( broadcast ) );
            return true;
        },
        __FILE__,
        __LINE__,
        std::chrono::milliseconds( 10 ) );

    ScenesExpressionsFixture::TearDown();
}

void SceneStopMeshbotTests::startOnUniquePorts( HttpServer& server ) {
    Process::SharedMutex processMutex( "uniquePortMutex" );
    std::unique_lock< Process::SharedMutex > lock( processMutex );
    int insecurePort = system_utils::getUniqueFreeLocalPort();
    server.setPort( insecurePort );
    int securePort = system_utils::getUniqueFreeLocalPort();

    server.setSecurePort( securePort );
    server.start();
    lock.unlock();
}

HttpServer& SceneStopMeshbotTests::getHttpServer() {
    static HttpServer lws;
    return lws;
}

void SceneStopMeshbotTests::SetUpTestCase() {
    curl_global_init( CURL_GLOBAL_ALL );
    startOnUniquePorts( getHttpServer() );
}

void SceneStopMeshbotTests::sendStop( const std::string& sceneId, const std::string& type, int codeLine ) {
    auto codeLineStr = getCodeLineString( codeLine );

    auto request =
        replaceTokens( stopSceneRequest, { { "__ID__", sceneId }, { "__BLOCK__", type }, { "__GROUP__", "" } } );
    auto answer = sendRequestAndCheckAnswer( request, codeLine );
    const auto& error = answer[ "error" ];
    EXPECT_TRUE( error.empty() ) << codeLineStr;
}

std::string SceneStopMeshbotTests::createActionRequestWithExecPolicy( std::string execPolicy ) {
    return replaceTokensInString( sendHttpRequestThenBlock,
                                  { { "__URL__", makeUrl( "/reflect" ) },
                                    { "__USER__", "" },
                                    { "__PASSWORD__", "" },
                                    { "__REQUEST_TYPE__", "GET" },
                                    { "__CONTENT_TYPE__", "text/plain" },
                                    { "__CONTENT__", "" },
                                    { "__HEADERS__", "" },
                                    { "__SKIP_SECURITY__", "true" },
                                    { "__EXEC_POLICY__", execPolicy } } );
}

std::string SceneStopMeshbotTests::createDelayedActionRequestWithExecPolicy( const std::string& itemId,
                                                                             std::string execPolicy ) {
    return replaceTokensInString( toggleItemValueDelayedThenBlock,
                                  { { "__VALUE__", itemId }, { "__EXEC_POLICY__", execPolicy } } );
}

JsonObject SceneStopMeshbotTests::sceneRequestWithFewActions( const std::string& actionBlock ) {
    return replaceTokens( createSceneRequest,
                          { { "__NAME__", "scene" },
                            { "__WHEN_BLOCKS__", "" },
                            { "__THEN_BLOCKS__",
                              actionBlock + "," + actionBlock + "," + actionBlock + "," + actionBlock + "," +
                                  actionBlock + "," + actionBlock } } );
}

void SceneStopMeshbotTests::stopDelayedActionsWithExecPolicy( std::string execPolicy, int codeLine ) {
    auto deviceId = makeDeviceSiren( __LINE__ );
    using namespace CoreD::Model;
    const std::string itemName = "sound_volume";

    Item item = makeIntItem( deviceId, itemName, 0 );
    auto itemId = addItem( m_remote, item, codeLine, nullptr );

    std::string actionBlock = createDelayedActionRequestWithExecPolicy( itemId, execPolicy );

    auto request = sceneRequestWithFewActions( actionBlock );

    std::string sceneId = createScene( request );
    runSceneManually( sceneId, codeLine );
    testRunProgressBroadcast( request, sceneId, "started", codeLine );

    sendStop( sceneId, "", codeLine );

    testRunProgressBroadcast( request, sceneId, "stopped", codeLine );

    checkNoBroadcastReceived( "", codeLine, std::chrono::seconds( 2 ) );
    deleteScene( sceneId );
}

void SceneStopMeshbotTests::checkRunContextRecords( int codeLine ) {
    auto codeLineStr = getCodeLineString( codeLine );

    const auto& dbClient = nma().getDBClient();
    auto contextRecords =
        dbClient.findAll( "temp.db.firmware.scenes_manage.execution_context", JSON_EMPTY_OBJ, nullptr );
    EXPECT_FALSE( contextRecords.size() > 0 ) << codeLineStr;
}

TEST_F( SceneStopMeshbotTests, scenesStopSuccess ) {  // 1.1
    auto request =
        replaceTokens( createSceneRequest,
                       { { "__NAME__", "scenesStopSuccess" }, { "__WHEN_BLOCKS__", "" }, { "__THEN_BLOCKS__", "" } } );

    auto sceneId = createScene( request, __LINE__ );
    request = replaceTokens( stopSceneRequest, { { "__ID__", sceneId }, { "__BLOCK__", "" }, { "__GROUP__", "" } } );
    auto answer = sendRequestAndCheckAnswer( request, __LINE__ );
    const auto& error = answer[ "error" ];
    EXPECT_TRUE( error.empty() );

    checkRunContextRecords( __LINE__ );

    deleteScene( sceneId );
}

TEST_F( SceneStopMeshbotTests, scenesStopFailed ) {  // 1.2
    auto request =
        replaceTokens( createSceneRequest,
                       { { "__NAME__", "scenesStopSuccess" }, { "__WHEN_BLOCKS__", "" }, { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId = createScene( request, __LINE__ );
    request = replaceTokens( stopSceneRequest, { { "__ID__", "sceneId" }, { "__BLOCK__", "" }, { "__GROUP__", "" } } );

    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    EXPECT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.scenes.not.exist" );
    EXPECT_EQ( json_get_str( error, "message" ), "The scene with this id does not exist" );

    answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.scenes.stop" }, { "params", JSON_EMPTY_OBJ } } );
    error = answer[ "error" ];
    EXPECT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32600 );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.notfound.sceneId" );
    EXPECT_EQ( json_get_str( error, "message" ), "Bad request, sceneId does not exist" );

    checkRunContextRecords( __LINE__ );
}

TEST_F( SceneStopMeshbotTests, sceneWithFewHttpRequestActionsCheckContext )  // 2.1
{
    Tests::HttpServer& server = getHttpServer();

    auto actionBlock = replaceTokensInString( sendHttpRequestThenBlock,
                                              { { "__URL__", makeUrl( "/reflect" ) },
                                                { "__USER__", "" },
                                                { "__PASSWORD__", "" },
                                                { "__REQUEST_TYPE__", "GET" },
                                                { "__CONTENT_TYPE__", "text/plain" },
                                                { "__CONTENT__", "" },
                                                { "__HEADERS__", "" },
                                                { "__SKIP_SECURITY__", "true" } } );

    auto sceneRequest = replaceTokens( createSceneRequest,
                                       { { "__NAME__", "HttpRequest" },
                                         { "__WHEN_BLOCKS__", "" },
                                         { "__THEN_BLOCKS__",
                                           actionBlock + "," + actionBlock + "," + actionBlock + "," + actionBlock +
                                               "," + actionBlock + "," + actionBlock } } );
    SceneID sceneId = createScene( sceneRequest );
    runSceneManually( sceneId, __LINE__ );
    testRunProgressBroadcast( sceneRequest, sceneId, "started", __LINE__ );

    const auto& dbClient = nma().getDBClient();
    auto contexts = dbClient.findAll( HaTempDBSceneExecutionContextDBName, JSON_EMPTY_OBJ, nullptr );

    EXPECT_EQ( contexts.size(), 1 );
    EXPECT_EQ( contexts[ 0 ][ "actions" ].size(), 6 );

    sendStop( sceneId, "", __LINE__ );

    testRunProgressBroadcast( sceneRequest, sceneId, "stopped", __LINE__ );
    checkNoBroadcastReceived( "", __LINE__, std::chrono::seconds( 1 ) );
    contexts = dbClient.findAll( HaTempDBSceneExecutionContextDBName, JSON_EMPTY_OBJ, nullptr );
    EXPECT_EQ( contexts.size(), 0 );
    checkRunContextRecords( __LINE__ );
    deleteScene( sceneId );
}

TEST_F( SceneStopMeshbotTests, stopSceneWithCheckResultExecPolicyActions )  // 3.1
{
    Tests::HttpServer& server = getHttpServer();
    auto actionBlock = createActionRequestWithExecPolicy( "check_result" );
    auto sceneRequest = sceneRequestWithFewActions( actionBlock );
    SceneID sceneId = createScene( sceneRequest );
    // start 6 delayed actions
    runSceneManually( sceneId, __LINE__ );
    testRunProgressBroadcast( sceneRequest, sceneId, "started", __LINE__ );

    const auto& dbClient = nma().getDBClient();
    auto contexts = dbClient.findAll( HaTempDBSceneExecutionContextDBName, JSON_EMPTY_OBJ, nullptr );

    // one action was started
    EXPECT_EQ( contexts.size(), 1 );
    EXPECT_EQ( contexts[ 0 ][ "actions" ].size(), 1 );

    // we stop next other 5 actions
    auto request =
        replaceTokens( stopSceneRequest, { { "__ID__", sceneId }, { "__BLOCK__", "" }, { "__GROUP__", "" } } );
    auto answer = sendRequestAndCheckAnswer( request, __LINE__ );
    const auto& error = answer[ "error" ];
    EXPECT_TRUE( error.empty() );

    testRunProgressBroadcast( sceneRequest, sceneId, "stopped", __LINE__ );
    checkNoBroadcastReceived( "", __LINE__, std::chrono::seconds( 1 ) );
    // check that context was deleted
    contexts = dbClient.findAll( HaTempDBSceneExecutionContextDBName, JSON_EMPTY_OBJ, nullptr );
    EXPECT_EQ( contexts.size(), 0 );

    checkRunContextRecords( __LINE__ );
    deleteScene( sceneId );
}

TEST_F( SceneStopMeshbotTests, stopSceneWithWaitResultExecPolicyActions )  // 3.2
{
    Tests::HttpServer& server = getHttpServer();
    auto actionBlock = createActionRequestWithExecPolicy( "wait_result" );
    auto sceneRequest = sceneRequestWithFewActions( actionBlock );
    SceneID sceneId = createScene( sceneRequest );

    // start 6 delayed actions
    runSceneManually( sceneId, __LINE__ );
    testRunProgressBroadcast( sceneRequest, sceneId, "started", __LINE__ );

    const auto& dbClient = nma().getDBClient();
    auto contexts = dbClient.findAll( HaTempDBSceneExecutionContextDBName, JSON_EMPTY_OBJ, nullptr );

    // one action was started
    EXPECT_EQ( contexts.size(), 1 );
    EXPECT_EQ( contexts[ 0 ][ "actions" ].size(), 1 );

    // we stop next other 5 actions
    auto request =
        replaceTokens( stopSceneRequest, { { "__ID__", sceneId }, { "__BLOCK__", "" }, { "__GROUP__", "" } } );
    auto answer = sendRequestAndCheckAnswer( request, __LINE__ );
    const auto& error = answer[ "error" ];
    EXPECT_TRUE( error.empty() );

    testRunProgressBroadcast( sceneRequest, sceneId, "stopped", __LINE__ );

    checkNoBroadcastReceived( "", __LINE__, std::chrono::seconds( 1 ) );
    // check that context was deleted
    contexts = dbClient.findAll( HaTempDBSceneExecutionContextDBName, JSON_EMPTY_OBJ, nullptr );
    EXPECT_EQ( contexts.size(), 0 );

    checkRunContextRecords( __LINE__ );
    deleteScene( sceneId );
}

TEST_F( SceneStopMeshbotTests, stopSceneWithIgnoreResultExecPolicyActions )  // 3.3
{
    Tests::HttpServer& server = getHttpServer();
    auto actionBlock = createActionRequestWithExecPolicy( "ignore_result" );

    auto sceneRequest = sceneRequestWithFewActions( actionBlock );
    SceneID sceneId = createScene( sceneRequest );

    // start 6 delayed actions
    runSceneManually( sceneId, __LINE__ );
    testRunProgressBroadcast( sceneRequest, sceneId, "started", __LINE__ );

    const auto& dbClient = nma().getDBClient();
    auto contexts = dbClient.findAll( HaTempDBSceneExecutionContextDBName, JSON_EMPTY_OBJ, nullptr );

    // 6 action was started
    EXPECT_EQ( contexts.size(), 1 );
    EXPECT_EQ( contexts[ 0 ][ "actions" ].size(), 6 );

    // we stop next other 5 actions
    auto request =
        replaceTokens( stopSceneRequest, { { "__ID__", sceneId }, { "__BLOCK__", "" }, { "__GROUP__", "" } } );
    auto answer = sendRequestAndCheckAnswer( request, __LINE__ );
    const auto& error = answer[ "error" ];
    EXPECT_TRUE( error.empty() );
    testRunProgressBroadcast( sceneRequest, sceneId, "stopped", __LINE__ );
    checkNoBroadcastReceived( "", __LINE__, std::chrono::seconds( 1 ) );
    // check that context was deleted
    contexts = dbClient.findAll( HaTempDBSceneExecutionContextDBName, JSON_EMPTY_OBJ, nullptr );
    EXPECT_EQ( contexts.size(), 0 );

    deleteScene( sceneId );
}

TEST_F( SceneStopMeshbotTests, checkStopSynckActions )  // 3.4
{
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const std::string expressionName1 = "var1";
    const std::string expressionName2 = "var2";
    const std::string urlPrefix1 = "/reflect?name=";
    const std::string urlPrefix2 = "/reflect?__delayed=2000&name=";

    Tests::HttpServer& server = getHttpServer();

    auto makeActionBlock = [this]( const std::string& urlPrefix,
                                   const std::string& expressionName,
                                   const std::string execPolicy ) -> std::string {
        const auto url = urlPrefix + expressionName;
        return replaceTokensInString( sendHttpRequestSaveResultThenBlock,
                                      { { "__URL__", makeUrl( url.c_str() ) },
                                        { "__USER__", "" },
                                        { "__PASSWORD__", "" },
                                        { "__REQUEST_TYPE__", "GET" },
                                        { "__CONTENT_TYPE__", "text/plain" },
                                        { "__CONTENT__", "" },
                                        { "__HEADERS__", "" },
                                        { "__SKIP_SECURITY__", "true" },
                                        { "__EXEC_POLICY__", execPolicy },
                                        { "__GLOBAL_VARIABLE_NAME__", expressionName } } );
    };

    auto actionBlock1 = makeActionBlock( urlPrefix1, expressionName1, "wait_result" );
    auto actionBlock2 = makeActionBlock( urlPrefix2, expressionName2, "wait_result" );
    auto actionBlock3 = makeActionBlock( urlPrefix2, expressionName2, "check_result" );

    bool isExpectedBroadcast = true;
    scenesVariableExpressionSetType< std::string >(
        expressionName1, "a", "hub.expression.added", "string", {}, isExpectedBroadcast, __LINE__ );

    scenesVariableExpressionSetType< std::string >(
        expressionName2, "b", "hub.expression.added", "string", {}, isExpectedBroadcast, __LINE__ );

    auto request =
        replaceTokens( createSceneRequest,
                       { { "__NAME__", "HttpRequestSave" },
                         { "__WHEN_BLOCKS__", "" },
                         { "__THEN_BLOCKS__", std::move( actionBlock1 + "," + actionBlock2 + "," + actionBlock3 ) } } );

    SceneID sceneId = createScene( request );
    runSceneManually( sceneId, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    sendStop( sceneId, "", __LINE__ );

    checkBroadcasts(
        { [this, &sceneId]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, sceneId, "stopped" ); },
          [expressionName1]( const Json& broadcast ) {
              return compareBroadcastAndLookingForFields(
                  broadcast, "", "hub.expression.changed", { { "name", expressionName1 } }, "" );
          } },
        __LINE__ );

    Json answer;
    JsonArray expressions;
    runExpressionsList( answer, false );
    checkNoError( answer, __FILE__, __LINE__ );
    checkExpressions( answer, expressions, __LINE__ );

    deleteSceneExpression( expressionName1, __LINE__ );
    deleteSceneExpression( expressionName2, __LINE__ );

    checkRunContextRecords( __LINE__ );
    deleteScene( sceneId );
}

TEST_F( SceneStopMeshbotTests, checkStopAsyncActions )  // 3.5
{
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const std::string expressionName1 = "var1";
    const std::string expressionName2 = "var2";
    const std::string urlPrefix1 = "/reflect?__delayed=2000&name=";
    const std::string urlPrefix2 = "/reflect?__delayed=4000&name=";

    Tests::HttpServer& server = getHttpServer();

    auto makeActionBlock = [this]( const std::string& urlPrefix,
                                   const std::string& expressionName,
                                   const std::string execPolicy ) -> std::string {
        const auto url = urlPrefix + expressionName;
        return replaceTokensInString( sendHttpRequestSaveResultThenBlock,
                                      { { "__URL__", makeUrl( url.c_str() ) },
                                        { "__USER__", "" },
                                        { "__PASSWORD__", "" },
                                        { "__REQUEST_TYPE__", "GET" },
                                        { "__CONTENT_TYPE__", "text/plain" },
                                        { "__CONTENT__", "" },
                                        { "__HEADERS__", "" },
                                        { "__SKIP_SECURITY__", "true" },
                                        { "__EXEC_POLICY__", execPolicy },
                                        { "__GLOBAL_VARIABLE_NAME__", expressionName } } );
    };

    auto actionBlock1 = makeActionBlock( urlPrefix1, expressionName1, "ignore_result" );
    auto actionBlock2 = makeActionBlock( urlPrefix2, expressionName2, "ignore_result" );

    bool isExpectedBroadcast = true;
    scenesVariableExpressionSetType< std::string >(
        expressionName1, "a", "hub.expression.added", "string", {}, isExpectedBroadcast, __LINE__ );

    scenesVariableExpressionSetType< std::string >(
        expressionName2, "b", "hub.expression.added", "string", {}, isExpectedBroadcast, __LINE__ );

    auto request = replaceTokens( createSceneRequest,
                                  { { "__NAME__", "HttpRequestSave" },
                                    { "__WHEN_BLOCKS__", "" },
                                    { "__THEN_BLOCKS__", std::move( actionBlock1 + "," + actionBlock2 ) } } );

    SceneID sceneId = createScene( request );
    runSceneManually( sceneId, __LINE__ );

    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    sendStop( sceneId, "", __LINE__ );

    checkBroadcasts(
        { [this, &sceneId]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, sceneId, "stopped" ); },
          [expressionName1]( const Json& broadcast ) {
              return compareBroadcastAndLookingForFields(
                  broadcast, "", "hub.expression.changed", { { "name", expressionName1 } }, "" );
          },
          [expressionName2]( const Json& broadcast ) {
              return compareBroadcastAndLookingForFields(
                  broadcast, "", "hub.expression.changed", { { "name", expressionName2 } }, "" );
          } },
        __LINE__ );

    Json answer;
    JsonArray expressions;
    runExpressionsList( answer, false );
    checkNoError( answer, __FILE__, __LINE__ );
    checkExpressions( answer, expressions, __LINE__ );

    deleteSceneExpression( expressionName1, __LINE__ );
    deleteSceneExpression( expressionName2, __LINE__ );

    checkRunContextRecords( __LINE__ );
    deleteScene( sceneId );
}

TEST_F( SceneStopMeshbotTests, checkStopAtionsWithOtherBlock )  // 3.6
{
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const std::string expressionName1 = "var1";
    const std::string expressionName2 = "var2";
    const std::string urlPrefix1 = "/reflect?name=";
    const std::string urlPrefix2 = "/reflect?__delayed=2000&name=";

    Tests::HttpServer& server = getHttpServer();

    auto makeActionBlock = [this]( const std::string& urlPrefix,
                                   const std::string& expressionName,
                                   const std::string execPolicy ) -> std::string {
        const auto url = urlPrefix + expressionName;
        return replaceTokensInString( sendHttpRequestSaveResultThenBlock,
                                      { { "__URL__", makeUrl( url.c_str() ) },
                                        { "__USER__", "" },
                                        { "__PASSWORD__", "" },
                                        { "__REQUEST_TYPE__", "GET" },
                                        { "__CONTENT_TYPE__", "text/plain" },
                                        { "__CONTENT__", "" },
                                        { "__HEADERS__", "" },
                                        { "__SKIP_SECURITY__", "true" },
                                        { "__EXEC_POLICY__", execPolicy },
                                        { "__GLOBAL_VARIABLE_NAME__", expressionName } } );
    };

    auto actionBlock1 = makeActionBlock( urlPrefix1, expressionName1, "wait_result" );
    auto actionBlock2 = makeActionBlock( urlPrefix2, expressionName2, "wait_result" );

    bool isExpectedBroadcast = true;
    scenesVariableExpressionSetType< std::string >(
        expressionName1, "a", "hub.expression.added", "string", {}, isExpectedBroadcast, __LINE__ );

    scenesVariableExpressionSetType< std::string >(
        expressionName2, "b", "hub.expression.added", "string", {}, isExpectedBroadcast, __LINE__ );

    auto request = replaceTokens( createSceneRequest,
                                  { { "__NAME__", "HttpRequestSave" },
                                    { "__WHEN_BLOCKS__", "" },
                                    { "__THEN_BLOCKS__", std::move( actionBlock1 + "," + actionBlock2 ) } } );

    SceneID sceneId = createScene( request );
    runSceneManually( sceneId, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    // first action will started
    sendStop( sceneId, "else", __LINE__ );

    checkBroadcasts( { [this, &sceneId]( const Json& broadcast ) {
                          return isRunProgressBroadcast( broadcast, sceneId, "finished" );
                      },
                       [expressionName1]( const Json& broadcast ) {
                           return compareBroadcastAndLookingForFields(
                               broadcast, "", "hub.expression.changed", { { "name", expressionName1 } }, "" );
                       },
                       [expressionName2]( const Json& broadcast ) {
                           return compareBroadcastAndLookingForFields(
                               broadcast, "", "hub.expression.changed", { { "name", expressionName2 } }, "" );
                       } },
                     __LINE__ );

    deleteSceneExpression( expressionName1, __LINE__ );
    deleteSceneExpression( expressionName2, __LINE__ );

    checkRunContextRecords( __LINE__ );
    deleteScene( sceneId );
}

TEST_F( SceneStopMeshbotTests, stopDelayedAction )  // 4.1
{
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );

    auto deviceId = makeDeviceSiren( __LINE__ );
    using namespace CoreD::Model;
    const std::string itemName = "sound_volume";

    Item item = makeIntItem( deviceId, itemName, 0 );
    auto itemId = addItem( m_remote, item, __LINE__, nullptr );

    std::string actionBlock = replaceTokensInString( toggleItemValueDelayedThenBlock, { { "__VALUE__", itemId } } );

    Json request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "ToggleValue" }, { "__WHEN_BLOCKS__", "" }, { "__THEN_BLOCKS__", std::move( actionBlock ) } } );

    std::string sceneId = createScene( request );
    runSceneManually( sceneId, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );

    sendStop( sceneId, "", __LINE__ );

    testRunProgressBroadcast( request, sceneId, "stopped", __LINE__ );
    checkNoBroadcastReceived( "", __LINE__, std::chrono::seconds( 5 ) );
    deleteScene( sceneId );
}

TEST_F( SceneStopMeshbotTests, stopDelayedActionsCheckResulttExecPolicy )  // 4.2
{
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    stopDelayedActionsWithExecPolicy( "check_result", __LINE__ );
}

TEST_F( SceneStopMeshbotTests, stopDelayedActionsIgnoreResulttExecPolicy )  // 4.3
{
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    stopDelayedActionsWithExecPolicy( "ignore_result", __LINE__ );
}

TEST_F( SceneStopMeshbotTests, stopDelayedActionsWaitResultExecPolicy )  // 4.4
{
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    stopDelayedActionsWithExecPolicy( "wait_result", __LINE__ );
}

TEST_F( SceneStopMeshbotTests, IsSceneStateStoppedTrigger )  // 5.1
{
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );

    auto deviceId = makeDeviceSiren( __LINE__ );
    using namespace CoreD::Model;
    const std::string itemName = "sound_volume";

    Item item = makeIntItem( deviceId, itemName, 0 );
    auto itemId = addItem( m_remote, item, __LINE__, nullptr );

    std::string actionBlock = replaceTokensInString( toggleItemValueDelayedThenBlock, { { "__VALUE__", itemId } } );

    Json request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "ToggleValue" }, { "__WHEN_BLOCKS__", "" }, { "__THEN_BLOCKS__", std::move( actionBlock ) } } );

    std::string sceneId1 = createScene( request );

    auto isSceneState =
        replaceTokensInString( isSceneStateWhenBlock, { { "__SCENE_ID__", sceneId1 }, { "__STATE__", "stopped" } } );
    auto request2 = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "isSceneState" }, { "__WHEN_BLOCKS__", isSceneState }, { "__THEN_BLOCKS__", "" } } );
    auto sceneId2 = createScene( request2, __LINE__ );

    runSceneManually( sceneId1, __LINE__ );
    testRunProgressBroadcast( request, sceneId1, "started", __LINE__ );
    sendStop( sceneId1, "", __LINE__ );
    testRunProgressBroadcast( request, sceneId1, "stopped", __LINE__ );

    testRunProgressBroadcast( request2, sceneId2, "started", __LINE__ );
    testRunProgressBroadcast( request2, sceneId2, "finished", __LINE__ );

    checkRunContextRecords( __LINE__ );
    deleteScene( sceneId1 );
}

}  // namespace Tests
}  // namespace HubPlatform
