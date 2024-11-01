#include "fixtures/scenes_expressions_fixture.h"
#include "common_test_resources/resources/scene_manager_resources.h"
#include "scenes_manager/remote_interface/scenes_remote_interface.h"
#include "database_client/dbqueryparser.h"
#include "scenes_manager/tests/fixtures/scenes_api_fixture.h"
#include "scenes_manager/tests/resources/generic_scene_json.h"

/*

TESTPLAN

DONE    1. get_expression_value lua method
DONE        1.1 wrong params
DONE        1.2 missed 'options' parameter
DONE        1.3 get expression value
DONE            1.3.1 get expression value, wrong parameters
DONE        1.4 separate compact modes
DONE        1.5 error if expression doesn't exist
DONE    2.  get_scene_info lua method
DONE        2.1 success case
DONE        2.2 scene doesn't exist case (fail)
DONE    3.  get_when_block_state lua method
DONE        3.1 success with latch
DONE        3.2 get state for "NOT" function
DONE        3.3 scene doesn't exist
DONE        3.4 block doesn't exist
DONE        3.5 state of an exception block
DONE        4.1 set expression lua action
DONE        4.2 set variable lua action
DONE        4.3 set scene enabled test
DONE        5.1 reset all latches test
DONE        5.2 reset latch test
DONE    6. scene stop test
DONE        6.1 invalid sceneId test
DONE        6.2 invalid block test
DONE        6.3 invalid groupId test
DONE        6.4 valid groupId wrong scene test
 */

namespace HubPlatform {
namespace Tests {

class ScenesBindingsTests : public ScenesExpressionsFixture {
public:
    void SetUp() override;
    void TearDown() override;

    std::string createItem();
};

void ScenesBindingsTests::SetUp() {
    ScenesExpressionsFixture::SetUp();
    clearExistingDevices( __LINE__ );
    callingFile( __FILE__ );
}

void ScenesBindingsTests::TearDown() {
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
    }
    ScenesExpressionsFixture::TearDown();
    ASSERT_EQ( expressions.size(), 0 ) << "Expressions list should be empty";
}

std::string ScenesBindingsTests::createItem() {
    using namespace CoreD::Model;

    const std::string deviceId = makeDeviceThermostat( __LINE__ );
    const std::string targetItemName = "item_from_scene_block";
    const int targetValue = 2;
    Item item = makeTemperatureItem( deviceId, targetItemName, targetValue, ValueScales::Fahrenheit );
    return addItem( m_remote, item, __LINE__, nullptr );
}

namespace Resources {
static const char* HUB_EXPRESSION_ADDED = "hub.expression.added";
}

TEST_F( ScenesBindingsTests, getExpressionWrongParams_1_1 ) /* 1.1 */ {
    bool isExpectedBroadcast = true;
    std::string expressionName = "expressionName9_1";
    scenesVariableExpressionSetType< std::string >( expressionName,
                                                    "This is a test string",
                                                    Resources::HUB_EXPRESSION_ADDED,
                                                    "string",
                                                    JSON( "key"
                                                          << "value" ),
                                                    isExpectedBroadcast,
                                                    __LINE__ );
    using namespace CoreD::Model::Resources;
    std::string scriptPath = "HUB:scenes_test/scripts/get_expression_value";
    Json params = JSON_EMPTY_OBJ;
    runScriptWithoutChecks( scriptPath, params, __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "failed", __LINE__ );

    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesBindingsTests, getExpressionOptionsNotExist_1_2 ) /* 1.2 */ {
    const std::string expressionName = "getExpressionValueLua1";
    bool isExpectedBroadcast = true;
    scenesVariableExpressionSetType< std::string >( expressionName,
                                                    "This is a test string",
                                                    Resources::HUB_EXPRESSION_ADDED,
                                                    "string",
                                                    JSON( "key"
                                                          << "value" ),
                                                    isExpectedBroadcast,
                                                    __LINE__ );

    using namespace CoreD::Model::Resources;

    std::string scriptPath = "HUB:scenes_test/scripts/get_expression_value";

    auto options = JSON( "size" << 10 << "offset" << 0 << "compact" << true );
    Json params = JSON( "name" << expressionName );
    runScriptWithoutChecks( scriptPath, params, __LINE__ );

    auto broadcast = waitForBroadcast( "hub.extensions.plugin.ui_broadcast", getCodeLineString( __LINE__ ) );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );

    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesBindingsTests, getExpressionValueOptionsSize_1_3 ) /* 1.3 */ {
    ScenesManager::RemoteInterface sceneManagerRemoteInterface( nma() );
    const std::string expressionName = "getExpressionValueLua";
    bool isExpectedBroadcast = true;
    std::string plainTextValue = "This is a test string";
    scenesVariableExpressionSetType< std::string >( expressionName,
                                                    plainTextValue,
                                                    Resources::HUB_EXPRESSION_ADDED,
                                                    "string",
                                                    JSON( "key"
                                                          << "value" ),
                                                    isExpectedBroadcast,
                                                    __LINE__ );

    auto options = JSON( "size" << 23 << "offset" << 0 << "compact" << true );
    const auto answer = sceneManagerRemoteInterface.getExpressionValue( expressionName, options );
    Json res;
    ASSERT_TRUE( json_get_obj( answer, "result", res ) ) << answer;

    JsonObject obj;
    ASSERT_EQ( json_get_str( res, "value" ), plainTextValue ) << res;
    ASSERT_EQ( json_get_bool( res, "last" ), true ) << res;
    ASSERT_EQ( json_get( res, "metadata", obj ), false ) << res;

    using namespace CoreD::Model::Resources;

    std::string scriptPath = "HUB:scenes_test/scripts/get_expression_value";

    options = JSON( "size" << 10 << "offset" << 0 );
    Json params = JSON( "name" << expressionName << "options" << options );
    runScriptWithoutChecks( scriptPath, params, __LINE__ );

    auto broadcast = waitForBroadcast( "hub.extensions.plugin.ui_broadcast", getCodeLineString( __LINE__ ) );

    JsonObject objMetadata;
    json_get_obj( broadcast, "result", obj );
    ASSERT_EQ( json_get_str( obj, "value" ), "This is a " );
    ASSERT_EQ( json_get_bool( obj, "last" ), false );
    ASSERT_EQ( json_get_str( obj, "value_type" ), "string" );
    ASSERT_EQ( json_get_bool( obj, "variable" ), true );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );

    options = JSON( "size" << 13 << "offset" << 10 );
    params = JSON( "name" << expressionName << "options" << options );
    runScriptWithoutChecks( scriptPath, params, __LINE__ );
    broadcast = waitForBroadcast( "hub.extensions.plugin.ui_broadcast", getCodeLineString( __LINE__ ) );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );

    json_get_obj( broadcast, "result", obj );
    ASSERT_EQ( json_get_str( obj, "value" ), "test string" );
    ASSERT_EQ( json_get_bool( obj, "last" ), true );
    ASSERT_EQ( json_get_str( obj, "value_type" ), "string" );
    ASSERT_EQ( json_get_bool( obj, "variable" ), true );

    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesBindingsTests, getExpressionValueOptionsInvalidParams_1_3_1 ) /* 1.3.1 */ {
    ScenesManager::RemoteInterface sceneManagerRemoteInterface( nma() );
    const std::string expressionName = "getExpressionValueLua";
    bool isExpectedBroadcast = true;
    std::string plainTextValue = "This is a test string";
    scenesVariableExpressionSetType< std::string >( expressionName,
                                                    plainTextValue,
                                                    Resources::HUB_EXPRESSION_ADDED,
                                                    "string",
                                                    JSON( "key"
                                                          << "value" ),
                                                    isExpectedBroadcast,
                                                    __LINE__ );

    auto options = JSON( "size" << 23 << "offset" << 0 << "compact" << true );
    const auto answer = sceneManagerRemoteInterface.getExpressionValue( expressionName, options );
    Json res;
    ASSERT_TRUE( json_get_obj( answer, "result", res ) ) << answer;

    JsonObject obj;
    ASSERT_EQ( json_get_str( res, "value" ), plainTextValue );
    ASSERT_EQ( json_get_bool( res, "last" ), true );
    ASSERT_EQ( json_get( res, "metadata", obj ), false );

    using namespace CoreD::Model::Resources;

    std::string scriptPath = "HUB:scenes_test/scripts/get_expression_value";

    int wrongOffset = plainTextValue.size() + 10;
    options = JSON( "size" << 10 << "offset" << wrongOffset );
    Json params = JSON( "name" << expressionName << "options" << options );
    runFailedScript( scriptPath, params, __LINE__ );

    wrongOffset = -1;
    options = JSON( "size" << 0 << "offset" << wrongOffset );
    params = JSON( "name" << expressionName << "options" << options );
    runFailedScript( scriptPath, params, __LINE__ );

    Json broadcast;
    // Should be no error, on error input, size will be set to the limit value
    int wrongSize = -1;
    options = JSON( "size" << wrongSize );
    params = JSON( "name" << expressionName << "options" << options );
    runScriptWithoutChecks( scriptPath, params, __LINE__ );
    broadcast = waitForBroadcast( "hub.extensions.plugin.ui_broadcast", getCodeLineString( __LINE__ ) );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );

    // More that a limit
    wrongSize = 500000 * 5;
    options = JSON( "size" << wrongSize );
    params = JSON( "name" << expressionName << "options" << options );
    runScriptWithoutChecks( scriptPath, params, __LINE__ );
    broadcast = waitForBroadcast( "hub.extensions.plugin.ui_broadcast", getCodeLineString( __LINE__ ) );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );

    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesBindingsTests, getExpressionValueOptionsCompact_1_4 ) /* 1.4 */ {
    ScenesManager::RemoteInterface sceneManagerRemoteInterface( nma() );

    const std::string expressionName = "getExpressionValueLua5";
    bool isExpectedBroadcast = true;
    scenesVariableExpressionSetType< std::string >( expressionName,
                                                    "This is a test string",
                                                    Resources::HUB_EXPRESSION_ADDED,
                                                    "string",
                                                    JSON( "key"
                                                          << "value" ),
                                                    isExpectedBroadcast,
                                                    __LINE__ );

    auto options = JSON( "size" << 23 << "offset" << 0 << "compact" << false );
    const auto answer = sceneManagerRemoteInterface.getExpressionValue( expressionName, options );
    Json res;
    ASSERT_TRUE( json_get_obj( answer, "result", res ) ) << answer;
    JsonObject obj;
    ASSERT_EQ( json_get( res, "metadata", obj ), true );

    options = JSON( "size" << 23 << "offset" << 0 << "compact" << true );
    res = sceneManagerRemoteInterface.getExpressionValue( expressionName, options );
    ASSERT_EQ( json_get( res, "metadata", obj ), false );

    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesBindingsTests, getExpressionNotExist_1_5 ) /* 1.5 */ {
    const std::string expressionName = "getExpressionValueLua6";
    bool isExpectedBroadcast = true;
    scenesVariableExpressionSetType< std::string >( expressionName,
                                                    "This is a test string",
                                                    Resources::HUB_EXPRESSION_ADDED,
                                                    "string",
                                                    JSON( "key"
                                                          << "value" ),
                                                    isExpectedBroadcast,
                                                    __LINE__ );

    using namespace CoreD::Model::Resources;

    std::string scriptPath = "HUB:scenes_test/scripts/get_expression_value";
    Json params = JSON( "name"
                        << "expressionName6" );
    runScriptWithoutChecks( scriptPath, params, __LINE__ );

    auto broadcast = waitForBroadcast( "hub.extensions.plugin.run.progress", getCodeLineString( __LINE__ ) );
    checkExtensionsPluginRunProgressBroadcast( "failed", __LINE__ );

    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesBindingsTests, getScenesInfoSuccess_2_1 ) /* 2.1 */ {
    ScenesManager::RemoteInterface sceneManagerRemoteInterface( nma() );
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );

    auto request = replaceTokens(
        createSceneRequest, { { "__NAME__", "getSceneInfo" }, { "__WHEN_BLOCKS__", "" }, { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId = createScene( request, __LINE__ );

    std::string scriptPath = "HUB:scenes_test/scripts/get_scene_info";
    Json params = JSON( "id" << sceneId );
    runScriptWithoutChecks( scriptPath, params, __LINE__ );

    JsonObject broadcast = waitForBroadcast( "hub.extensions.plugin.ui_broadcast", getCodeLineString( __LINE__ ) );

    JsonObject obj;
    ASSERT_TRUE( json_get_obj( broadcast, "result", obj ) );

    ASSERT_EQ( json_get_str( obj, "name" ), "getSceneInfo" );
    ASSERT_EQ( json_get_bool( obj, "enabled" ), true );
    ASSERT_EQ( json_get_str( obj, "room_id" ), "__PARENT_ID__" );
    ASSERT_EQ( json_get_str( obj, "exec_policy" ), "__EXEC_POLICY__" );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );
}

TEST_F( ScenesBindingsTests, getScenesInfoFail_2_2 ) /* 2.2 */ {
    ScenesManager::RemoteInterface sceneManagerRemoteInterface( nma() );
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );

    std::string scriptPath = "HUB:scenes_test/scripts/get_scene_info";
    Json params = JSON( "id"
                        << "sceneId" );
    runScriptWithoutChecks( scriptPath, params, __LINE__ );
    waitForBroadcast( "hub.extensions.plugin.run.progress", getCodeLineString( __LINE__ ) );
    checkExtensionsPluginRunProgressBroadcast( "failed", __LINE__ );
}

TEST_F( ScenesBindingsTests, getWhenBlockLatched_3_1 ) /* 3.1 */ {
    ScenesManager::RemoteInterface sceneManagerRemoteInterface( nma() );
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );

    const std::string itemId = createItem();

    std::string functionSettings = R"("latch": { "enabled":true, "name":"latchName"})";
    auto whenBlock = replaceTokensInString( compareNumbersWhenBlock,
                                            { { "__SOURCE_NAME__", "item" },
                                              { "__SOURCE_TYPE__", "item" },
                                              { "__SOURCE_VALUE__", "\"" + itemId + "\"" },
                                              { "__COMPARATOR__", ">" },
                                              { "__VALUE__", "0" } } );
    auto functionOperator = replaceTokensInString(
        functionOperatorWhenBlock, { { "__BLOCKS__", whenBlock }, { "__FUNCTION__", functionSettings } } );
    Json request = replaceTokens( createSceneRequest,
                                  { { "__NAME__", "SceneWithCompareNumbers" },
                                    { "__WHEN_BLOCKS__", functionOperator },
                                    { "__THEN_BLOCKS__", "" } } );

    Json scene;
    SceneID sceneId = createScene( request, scene, __LINE__ );

    Json blockId;
    ASSERT_TRUE( json_get( scene, { "when", 0, db::ID_FIELD_NAME }, blockId ) );

    std::string scriptPath = "HUB:scenes_test/scripts/get_when_block_state";
    Json params = JSON( "scene_id" << sceneId << "block_id" << json_get_str( blockId ) );

    runScriptWithoutChecks( scriptPath, params, __LINE__ );
    JsonObject broadcast = waitForBroadcast( "hub.extensions.plugin.ui_broadcast", getCodeLineString( __LINE__ ) );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );

    JsonObject result = JSON_EMPTY_OBJ;
    ASSERT_TRUE( json_get_obj( broadcast, "result", result ) );

    Json function = JSON( "is_latched" << false << "name"
                                       << "latch" );

    Json requestBlockId;
    ASSERT_TRUE( json_get( scene, { "when", 0, "fields", 0, "value", 0, db::ID_FIELD_NAME }, requestBlockId ) );
    Json expectedResult = JSON( "blocks" << JSON_ARRAY_( requestBlockId ) << "function" << function << "method"
                                         << "function"
                                         << "plugin"
                                         << "scenes_test"
                                         << "state"
                                         << "false" );
    ASSERT_EQ( result, expectedResult );
}

TEST_F( ScenesBindingsTests, getWhenBlockNot_3_2 ) /* 3.2 */ {
    ScenesManager::RemoteInterface sceneManagerRemoteInterface( nma() );
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );

    auto whenBlockTime = replaceTokensInString( isIntervalWhenBlock, { { "__INTERVAL__", "1h" } } );
    auto whenBlockNot = replaceTokensInString( notOperatorWhenBlock, { { "__BLOCK__", whenBlockTime } } );

    Json request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "SceneWithCompareNumbers" }, { "__WHEN_BLOCKS__", whenBlockNot }, { "__THEN_BLOCKS__", "" } } );

    Json scene;
    SceneID sceneId = createScene( request, scene, __LINE__ );
    Json blockId;
    ASSERT_TRUE( json_get( scene, { "when", 0, db::ID_FIELD_NAME }, blockId ) );

    std::string scriptPath = "HUB:scenes_test/scripts/get_when_block_state";
    Json params = JSON( "scene_id" << sceneId << "block_id" << json_get_str( blockId ) );

    runScriptWithoutChecks( scriptPath, params, __LINE__ );
    JsonObject broadcast = waitForBroadcast( "hub.extensions.plugin.ui_broadcast", getCodeLineString( __LINE__ ) );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );
    JsonObject result;
    ASSERT_TRUE( json_get_obj( broadcast, "result", result ) );
    ASSERT_EQ( json_get_str( result, "method" ), "not" );
    ASSERT_EQ( json_get_str( result, "state" ), "false" );

    Json childBlockId;
    ASSERT_TRUE( json_get( scene, { "when", 0, "fields", 0, "value", db::ID_FIELD_NAME }, childBlockId ) );

    JsonObject resultBlocks;
    json_get_arr( result, "blocks", resultBlocks );
    ASSERT_EQ( resultBlocks, JSON_ARRAY_( std::move( childBlockId ) ) );
}

TEST_F( ScenesBindingsTests, getWhenBlockSceneIdDoesntExist_3_3 ) /* 3.3 */ {
    ScenesManager::RemoteInterface sceneManagerRemoteInterface( nma() );
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );

    Json scene;
    SceneID sceneId = createScene( createSceneJson, scene, __LINE__ );
    Json blockId;
    ASSERT_TRUE( json_get( scene, { "when", 0, db::ID_FIELD_NAME }, blockId ) );
    std::string scriptPath = "HUB:scenes_test/scripts/get_when_block_state";
    Json params = JSON( "scene_id"
                        << "nonExistentSceneId"
                        << "block_id" << json_get_str( blockId ) );

    runScriptWithoutChecks( scriptPath, params, __LINE__ );
    waitForBroadcast( "hub.extensions.plugin.run.progress", getCodeLineString( __LINE__ ) );

    checkExtensionsPluginRunProgressBroadcast( "failed", __LINE__ );
}

TEST_F( ScenesBindingsTests, getWhenBlockBlockIdDoesntExist_3_4 ) /* 3.4 */ {
    ScenesManager::RemoteInterface sceneManagerRemoteInterface( nma() );
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );

    Json scene;
    SceneID sceneId = createScene( createSceneJson, scene, __LINE__ );
    std::string scriptPath = "HUB:scenes_test/scripts/get_when_block_state";
    Json params = JSON( "scene_id" << sceneId << "block_id"
                                   << "nonExistentBlockId" );

    runScriptWithoutChecks( scriptPath, params, __LINE__ );
    waitForBroadcast( "hub.extensions.plugin.run.progress", getCodeLineString( __LINE__ ) );

    checkExtensionsPluginRunProgressBroadcast( "failed", __LINE__ );
}

TEST_F( ScenesBindingsTests, getExceptionBlock_3_5 ) /* 3.5 */ {
    ScenesManager::RemoteInterface sceneManagerRemoteInterface( nma() );
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );

    const std::string itemId = createItem();
    Json request = replaceTokens( createSceneRequest, { { "__WHEN_BLOCKS__", "" }, { "__THEN_BLOCKS__", "" } } );
    Json exceptionBlock = replaceTokens( compareNumbersWhenBlock,
                                         { { "__SOURCE_NAME__", "item" },
                                           { "__SOURCE_TYPE__", "item" },
                                           { "__SOURCE_VALUE__", "\"" + itemId + "\"" },
                                           { "__COMPARATOR__", ">" },
                                           { "__VALUE__", "0" } } );
    request[ "params" ][ "exceptions" ] = JSON_ARRAY_( std::move( exceptionBlock ) );

    Json scene;
    SceneID sceneId = createScene( request, scene, __LINE__ );

    Json blockId;
    ASSERT_TRUE( json_get( scene, { "exceptions", 0, db::ID_FIELD_NAME }, blockId ) );

    const std::string scriptPath = "HUB:scenes_test/scripts/get_when_block_state";
    const Json params = JSON( "scene_id" << sceneId << "block_id" << json_get_str( blockId ) );

    runScriptWithoutChecks( scriptPath, params, __LINE__ );
    JsonObject broadcast = waitForBroadcast( "hub.extensions.plugin.ui_broadcast", getCodeLineString( __LINE__ ) );
    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );

    JsonObject result = JSON_EMPTY_OBJ;
    ASSERT_TRUE( json_get_obj( broadcast, "result", result ) );

    Json expected = R"({ "method": "compareNumbers", "blocks": {}, "plugin":"scenes_test", "state": "false" })"_json;
    ASSERT_EQ( result, expected );
}

TEST_F( ScenesBindingsTests, setExpression ) /* 4.1 */ {
    ScenesManager::RemoteInterface sceneManagerRemoteInterface( nma() );
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );

    const auto expressionName = test_info_->name();
    const auto expressionCode = "return 2 + 2 * 2";
    auto deviceId = makeDeviceSiren( __LINE__ );

    using namespace CoreD::Model;
    const std::string itemName = "sound_volume";
    const int value = 17;
    Item item = makeIntItem( deviceId, itemName, value );
    auto itemId = addItem( m_remote, item, __LINE__, nullptr );

    const char* code = "return params.sound_volume_quad *3";

    auto itemsClause = JSON( "name"
                             << "sound_volume_quad"
                             << "_id" << itemId );

    scenesExpressionSet( expressionName, expressionCode, {}, {}, false, __LINE__ );

    const std::string scriptPath = "HUB:scenes_test/scripts/set_expression";

    Json params;
    JsonArrayBuilder itemsArr;
    itemsArr << itemsClause;

    params[ "items" ] = itemsArr;
    std::string valueType = "int";
    code = "return params.sound_volume_quad *4";

    const auto metadata = JSON( "key"
                                << "value" );
    const Json scriptParams = JSON( "name" << expressionName << "params" << params << "value_type" << valueType
                                           << "metadata" << metadata << "code" << code );

    runScriptWithoutChecks( scriptPath, scriptParams, __LINE__ );

    JsonObject broadcast = waitForBroadcast( "hub.expression.changed", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.expression.changed" ) << getCodeLineString( __LINE__ );
    broadcast = waitForBroadcast( "hub.extensions.plugin.ui_broadcast", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.extensions.plugin.ui_broadcast" )
        << getCodeLineString( __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );

    Json answer, expressions;
    runExpressionsList( answer, false );
    checkExpressions( answer, expressions, __LINE__ );

    checkExpressionByName( expressions,
                           expressionName,
                           { { "code", code },
                             { "params", params },
                             { "valueType", valueType },
                             { "metadata", metadata },
                             { Resources::CheckBroadcast::HasNot, "error" } },
                           __LINE__ );

    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesBindingsTests, setVariable ) /* 4.2 */ {
    ScenesManager::RemoteInterface sceneManagerRemoteInterface( nma() );
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const auto expressionName = test_info_->name();
    scenesVariableExpressionSetType< int >( expressionName, 123, "hub.expression.added", "int", {}, true, __LINE__ );

    const std::string scriptPath = "HUB:scenes_test/scripts/set_variable";
    const auto value = 140;
    const auto valueType = "int";
    const auto metadata = JSON( "key"
                                << "value" );
    const Json scriptParams =
        JSON( "name" << expressionName << "value" << value << "value_type" << valueType << "metadata" << metadata );

    runScriptWithoutChecks( scriptPath, scriptParams, __LINE__ );

    JsonObject broadcast = waitForBroadcast( "hub.expression.changed", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.expression.changed" ) << getCodeLineString( __LINE__ );
    broadcast = waitForBroadcast( "hub.extensions.plugin.ui_broadcast", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.extensions.plugin.ui_broadcast" )
        << getCodeLineString( __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );

    Json answer, expressions;
    runExpressionsList( answer, false );
    checkExpressions( answer, expressions, __LINE__ );

    checkExpressionByName( expressions,
                           expressionName,
                           { { "value", value },
                             { "metadata", metadata },
                             { "valueType", valueType },
                             { Resources::CheckBroadcast::HasNot, "error" } },
                           __LINE__ );

    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesBindingsTests, setSceneEnabled ) /* 4.3 */ {
    ScenesManager::RemoteInterface sceneManagerRemoteInterface( nma() );
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );

    auto request = replaceTokens(
        createSceneRequest, { { "__NAME__", "getSceneInfo" }, { "__WHEN_BLOCKS__", "" }, { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId = createScene( request, __LINE__ );

    const std::string scriptPath = "HUB:scenes_test/scripts/set_scene_enabled";

    bool sceneEnabled = true;
    auto scriptParams = JSON( "scene_id" << sceneId << "enabled" << sceneEnabled );

    runScriptWithoutChecks( scriptPath, scriptParams, __LINE__ );

    checkChangedBroadcastEnableRequest( sceneId, request, sceneEnabled, __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );

    sceneEnabled = false;
    scriptParams = JSON( "scene_id" << sceneId << "enabled" << sceneEnabled );

    runScriptWithoutChecks( scriptPath, scriptParams, __LINE__ );

    checkChangedBroadcastEnableRequest( sceneId, request, sceneEnabled, __LINE__ );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );

    // Wrong sceneId case
    scriptParams = JSON( "scene_id"
                         << "wrongId"
                         << "enabled" << sceneEnabled );
    runScriptWithoutChecks( scriptPath, scriptParams, __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    const auto& broadcast = waitForBroadcast( "hub.extensions.plugin.run.progress", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.extensions.plugin.run.progress" )
        << getCodeLineString( __LINE__ );
    JsonObject result;
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) ) << getCodeLineString( __LINE__ );
    JsonObject error;
    EXPECT_TRUE( json_get_obj( result, "error", error ) ) << getCodeLineString( __LINE__ );
    EXPECT_TRUE( json_get_str( error, "reason" ).find( "The scene doesn't exist" ) != std::string::npos );

    deleteScene( sceneId, __LINE__ );
}

TEST_F( ScenesBindingsTests, resetStatusOfAllSceneBlocksTest ) /* 5.1 */ {
    Json answer;
    Json whenBlock = SceneEngine::UT::intervalCondition;
    Json whenBlocks;
    std::string functionOperatorStr = replaceTokensInString(
        SceneEngine::UT::functionOperatorWhenBlock,
        { { "__BLOCKS__", whenBlock.dump() }, { "__FUNCTION__", R"("latch": { "enabled": true })" } } );

    Json functionOperator = Json::parse( functionOperatorStr );
    whenBlocks = { functionOperator, functionOperator, functionOperator, functionOperator };

    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "resetLatchesScene" }, { "__WHEN_BLOCKS__", whenBlocks.dump() }, { "__THEN_BLOCKS__", "" } } );

    request[ "params" ][ "when" ] = whenBlocks;
    SceneID sceneId = createScene( request, __LINE__ );
    const std::string scriptPath = "HUB:scenes_test/scripts/reset_scene_latches";

    // success case
    auto scriptParams = JSON( "scene_id" << sceneId );
    runScriptWithoutChecks( scriptPath, scriptParams, __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );

    // fail scene id case
    scriptParams = JSON( "scene_id"
                         << "failSceneId" );
    runScriptWithoutChecks( scriptPath, scriptParams, __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcastByReason( "failed", "The scene doesn't exist", __LINE__ );
    deleteScene( sceneId, __LINE__ );
}

TEST_F( ScenesBindingsTests, resetStatusOfSceneBlockTest ) /* 5.2 */ {
    Json answer;
    Json whenBlock = SceneEngine::UT::intervalCondition;
    Json whenBlocks;
    const char* blockId = "blockId";
    auto functionOperatorStr = replaceTokensInString(
        SceneEngine::UT::functionOperatorWhenBlock,
        { { "__BLOCKS__", whenBlock.dump() }, { "__FUNCTION__", R"("latch": { "enabled": true })" } } );

    Json functionOperator = Json::parse( functionOperatorStr );
    functionOperator[ "_id" ] = blockId;
    whenBlocks = { functionOperator };

    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "resetLatchesScene" }, { "__WHEN_BLOCKS__", whenBlocks.dump() }, { "__THEN_BLOCKS__", "" } } );

    request[ "params" ][ "when" ] = whenBlocks;
    SceneID sceneId = createScene( request, __LINE__ );
    const std::string scriptPath = "HUB:scenes_test/scripts/reset_latch";

    // success case
    auto scriptParams = JSON( "scene_id" << sceneId << "block_id" << blockId );
    runScriptWithoutChecks( scriptPath, scriptParams, __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );

    // fail scene id case
    scriptParams = JSON( "scene_id"
                         << "failSceneId"
                         << "block_id" << blockId );
    runScriptWithoutChecks( scriptPath, scriptParams, __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcastByReason( "failed", "The scene doesn't exist", __LINE__ );

    // fail block id case
    scriptParams = JSON( "scene_id" << sceneId << "block_id"
                                    << "failBlockId" );
    runScriptWithoutChecks( scriptPath, scriptParams, __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcastByReason( "failed", "The block doesn't exist", __LINE__ );
}

TEST_F( ScenesBindingsTests, invalidSceneId ) /* 6.1 */ {
    auto sceneRequest = createEmptyScene;
    SceneID sceneId = createScene( sceneRequest );

    const std::string scriptPath = "HUB:scenes_test/scripts/stop_scene";

    auto scriptParams = JSON( "scene_id"
                              << "sceneId" );

    runScriptWithoutChecks( scriptPath, scriptParams, __LINE__ );
    auto broadcast = waitForBroadcast( "hub.extensions.plugin.ui_broadcast",
                                       NMA_TEST_MAKE_MESSAGE( "Waiting for stopScene broadcast" ) );

    EXPECT_EQ( broadcast[ "result" ][ "error" ][ "data" ], "\"The scene doesn't exist\"" );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );

    deleteScene( sceneId, __LINE__ );
}

TEST_F( ScenesBindingsTests, invalidBlock ) /* 6.2 */ {
    SceneID sceneId = sceneWithBlocks( __LINE__ );

    const std::string scriptPath = "HUB:scenes_test/scripts/stop_scene";

    auto scriptParams = JSON( "scene_id" << sceneId << "block"
                                         << "block" );

    runScriptWithoutChecks( scriptPath, scriptParams, __LINE__ );

    auto broadcast = waitForBroadcast( "hub.extensions.plugin.ui_broadcast",
                                       NMA_TEST_MAKE_MESSAGE( "Waiting for stopScene broadcast" ) );

    EXPECT_EQ( broadcast[ "result" ][ "error" ][ "data" ], "\"Invalid action block type\"" );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );

    deleteScene( sceneId, __LINE__ );
}

TEST_F( ScenesBindingsTests, invalidGroupId ) /* 6.3 */ {
    ScenesManager::RemoteInterface sceneManagerRemoteInterface( nma() );

    Json scene;
    SceneID sceneId = createScene( createSceneJson, scene, __LINE__ );
    std::string scriptPath = "HUB:scenes_test/scripts/stop_scene";
    Json params = JSON( "scene_id" << sceneId << "block"
                                   << "thenGroups"
                                   << "group"
                                   << "nonExistentBlockId" );

    runScriptWithoutChecks( scriptPath, params, __LINE__ );

    auto broadcast = waitForBroadcast( "hub.extensions.plugin.ui_broadcast",
                                       NMA_TEST_MAKE_MESSAGE( "Waiting for stopScene broadcast" ) );

    EXPECT_EQ( broadcast[ "result" ][ "error" ][ "data" ], "\"A group doesn't exist\"" );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );

    deleteScene( sceneId, __LINE__ );
}

TEST_F( ScenesBindingsTests, validGroupIdWrongScene ) /* 6.4 */ {
    auto dev1 = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );

    auto scene1Request = createEmptyScene;
    scene1Request[ "params" ][ "when" ].push_back( replaceTokens(
        isItemStateWhenBlock, { { "__ITEM_ID__", dev1.m_itemId }, { "__TYPE__", "int" }, { "__VALUE__", "10" } } ) );

    Json scene1CreateResult;
    auto scene1Id = createScene( scene1Request, scene1CreateResult, __LINE__ );
    auto groupId = json_get_str( scene1CreateResult[ "when" ][ 0 ], "_id" );

    Json scene;
    SceneID sceneId = createScene( createSceneJson, scene, __LINE__ );

    std::string scriptPath = "HUB:scenes_test/scripts/stop_scene";
    Json params = JSON( "scene_id" << sceneId << "block"
                                   << "thenGroups"
                                   << "group" << groupId );

    runScriptWithoutChecks( scriptPath, params, __LINE__ );
    auto broadcast = waitForBroadcast( "hub.extensions.plugin.ui_broadcast",
                                       NMA_TEST_MAKE_MESSAGE( "Waiting for stopScene broadcast" ) );

    EXPECT_EQ( broadcast[ "result" ][ "error" ][ "data" ], "\"The group doesn't belong to the scene\"" );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );

    deleteScene( scene1Id, __LINE__ );
    deleteScene( sceneId, __LINE__ );
}

}  // namespace Tests
}  // namespace HubPlatform
