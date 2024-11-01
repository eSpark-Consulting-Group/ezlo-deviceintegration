#include "fixtures/scenes_expressions_fixture.h"
#include "common_test_resources/fixtures/core_test_fixture.h"
#include "common_test_resources/fixtures/scene_manager_tests_fixture.h"
#include "common_test_resources/resources/scene_manager_resources.h"
#include "scenes_manager/remote_interface/scenes_remote_interface.h"
#include "cored/remote_interface/cored_remote_interface.h"
#include "database_client/dbqueryparser.h"
#include "common_utils/cpp14.h"
#include <gtest/gtest.h>
#include "test_utils/component_tests/http_server/http_server.h"
#include "test_utils/test_utils.h"
#include "system_utils/process/shared_mutex.h"
#include "system_utils/find_free_local_port.h"
#include <curl/curl.h>
#include "tiny/tiny_consolecolors.h"
#include "cored/model/values/values.h"

/*

TESTPLAN

DONE    1. Command hub.scenes.expressions.set
DONE        1.1 create new arithmetic expression, get broadcast hub.expression.added
DONE        1.2 update existing arithmetic expression, get broadcast hub.expression.changed
DONE        1.2.1 update existing arithmetic expression, with addition parameters (variable, valueType, metadata)
                get broadcast hub.expression.changed
DONE        1.2.2 update existing arithmetic expression with error
DONE        1.3 create new expression with syntax error
DONE        1.4 error: no item found
DONE        1.5 error: no device found
DONE        1.6 create good new expression with params.device_item_names
DONE        1.7 create good new expression with params.items
DONE        1.9 create new expression with wrong value type
DONE            1.9.1 try to create expression with type mismatch
DONE        1.10 create new expression with not empty params and 'variable'=true
DONE        1.11 change expression using setExpression actionBlock
DONE        1.12 set expression action block with parameters
DONE        1.13 set expression value
DONE        1.14 create expression with not found item error
DONE        1.15 create expression with error device name
DONE        1.16 create expression with long name error
DONE        1.17 create expression with mismatch type error

DONE    2. Command hub.scenes.expressions.delete
DONE        2.1 delete existing expression
DONE        2.2 delete nonexistent expression, error

DONE    3. Command hub.scenes.expressions.list
DONE        3.1 Command hub.scenes.expressions.list, default showCode(=false)
DONE        3.2 Command hub.scenes.expressions.list, showCode=true
DONE        3.3 Command hub.scenes.expressions.list with new fields: valueType, metadata and variable
DONE            3.3.1 check saved params
DONE            3.3.2 Command hub.scenes.expressions.list with filters
DONE            3.3.3 Command hub.scenes.expressions.list with code valueType metadata and variable filters
('valueType', 'variable')(check saved params)
DONE       3.4  Command hub.scenes.expressions.list with new fields: names, showValue
DONE            3.4.1  Command hub.scenes.expressions.list with separate 'names'
DONE            3.4.2  Command hub.scenes.expressions.list with separate 'showValue'
DONE    4. Event tracking
DONE        4.1 Item updated
DONE            4.1.1 Item value updated
DONE            4.1.2 Item name updated
DONE            4.1.3 Item removed
DONE            4.1.4 Previously removed item added back
DONE        4.2 Device name updated
DONE    5. Usage in scene blocks
DONE        5.1 isItemState with expression
DONE            5.1.1 item reaches expression value
DONE            5.1.2 expression reaches item value
DONE        5.2 compareNumbers with expression
DONE            5.2.1 item reaches expression value
DONE            5.2.2 expression reaches item value
DONE            5.2.3 expression reaches item
DONE            5.2.4 left expression reaches right expression
DONE            5.2.5 right expression reaches left expression
DONE        5.3 Expression does not exist error
DONE        5.4 isItemState for scalable values with expression
DONE            5.4.1 scalable item value reaches expression value
DONE            5.4.2 expression reaches item scalable value
DONE        5.5 compareNumbers for scalable values with expression
DONE            5.5.1 item scalable value reaches expression value
DONE            5.5.2 expression reaches item scalable value
DONE        5.6 compareString with expression
DONE            5.6.1 Item vs Expression
DONE            5.6.2 Expression vs Expression
DONE            5.6.3 Expression vs Item
DONE        5.7 stringOperations
DONE            5.7.1 stringOperation Item vs Expression
DONE            5.7.2 stringOperation Expression vs Expression
DONE            5.7.3 stringOperation Expression vs Item
DONE        5.8 compareValues
DONE            5.8.1 compareValues with Item vs Expression
DONE            5.8.2 compareValues with Expression vs Expression
DONE            5.8.3 compareValues Expression vs Item
DONE        5.9 isItemStateChanged with expression
DONE        5.10 combination of isItemStateChanged + isItemState + compareNumbers
DONE    6. Variable expression
DONE        6.1 variable expression set, create expression
DONE        6.2 variable expression list error
DONE        6.3 variable expression typed list
DONE        6.4 compare numbers with expression
DONE        6.5 variable expression scale not found error
DONE        6.6 variable expression inavlid scale error
DONE    7. saveGlobalVariable/sendHttpRequest
DONE        7.1 http request, save to nonexistent variable
DONE        7.2 http request, save result, ok
DONE        7.3 http request ended with error, save error
DONE        7.4 Two sendHTTPRequest action use the same global variable to save data test
DONE        7.5 save two results, with direct requests order
DONE        7.6 save two retults, with inverse requests order
DONE        7.7 http request ended with error, save error: check type saving
DONE    8. saveGlobalVariable/cloudAPI
DONE        8.1 save to nonexistent variable
DONE        8.2 save result, ok
DONE        8.3 ended with error, save error
DONE    9. bigDataTests
DONE        9.1 big data in expressions

DONE        9.2 create scene with big data in actions


 */

namespace HubPlatform {
namespace Tests {

class ScenesExpressionsTests : public ScenesExpressionsFixture {
public:
    ScenesExpressionsTests();
    void SetUp() override;
    void TearDown() override;
    static void SetUpTestCase();

    static void startOnUniquePorts( Tests::HttpServer& server );
    static HttpServer& getHttpServer();

protected:
    CoreDRemoteInterface::CoreDInterface& m_remote;
};

ScenesExpressionsTests::ScenesExpressionsTests()
: m_remote( makeRemote( nma() ) ) {}

void ScenesExpressionsTests::SetUp() {
    ScenesExpressionsFixture::SetUp();
    clearExistingDevices( __LINE__ );
    callingFile( __FILE__ );
    getPort() = getHttpServer().getPort();
}

void ScenesExpressionsTests::TearDown() {
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

void ScenesExpressionsTests::SetUpTestCase() {
    curl_global_init( CURL_GLOBAL_ALL );
    startOnUniquePorts( getHttpServer() );
}

namespace SceneCommands {
const auto expressionDelete =
    R"({ "method": "hub.scenes.expressions.delete", "params": { "name": "twoPlusTwoMultiplyTwo" }})";
const auto expressionList = R"({ "method": "hub.scenes.expressions.list", "params": {}})";
}  // namespace SceneCommands

void ScenesExpressionsTests::startOnUniquePorts( HttpServer& server ) {
    Process::SharedMutex processMutex( "uniquePortMutex" );
    std::unique_lock< Process::SharedMutex > lock( processMutex );
    int insecurePort = system_utils::getUniqueFreeLocalPort();
    server.setPort( insecurePort );
    int securePort = system_utils::getUniqueFreeLocalPort();

    server.setSecurePort( securePort );
    server.start();
    lock.unlock();
}

HttpServer& ScenesExpressionsTests::getHttpServer() {
    static HttpServer lws;
    return lws;
}

TEST_F( ScenesExpressionsTests, expressionSet_create_T1_1 ) /* 1.1 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const auto expressionName = test_info_->name();
    const auto code = "return 2 + 2 * 2";
    scenesExpressionSet( expressionName, code, {}, {}, false, __LINE__ );
    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, expressionSet_update_T1_2 ) /* 1.2 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const auto expressionName = test_info_->name();
    auto code = "return 5 + 5";
    scenesExpressionSet( expressionName, code, {}, {}, false, __LINE__ );

    // update existing expression
    code = "return 12 + 12 * 12";
    scenesExpressionSet( expressionName, code, {}, {}, true, __LINE__ );
    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, set_updateTyped_T1_2_1 ) /* 1.2.1 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const auto expressionName = test_info_->name();
    auto code = "return 5 + 5";
    auto meta1 = JSON( "meta"
                       << "1" );
    auto meta2 = JSON( "meta"
                       << "2" );
    auto meta3 = JSON( "meta"
                       << "3" );

    scenesExpressionSet( expressionName, code, {}, {}, false, __LINE__ );
    std::string broadcastName = "hub.expression.changed";
    scenesExpressionSetExt( expressionName, code, {}, {}, true, "int", meta1, false, false, __LINE__ );

    checkBroadcast( "",
                    "hub.expression.changed",
                    { { "name", expressionName },
                      { Resources::CheckBroadcast::Has, "code" },
                      { Resources::CheckBroadcast::Has, "valueType" },
                      { Resources::CheckBroadcast::Has, "metadata" },
                      { Resources::CheckBroadcast::HasNot, "variable" } },
                    __LINE__ );

    scenesExpressionSetExt( expressionName, code, {}, {}, true, "float", meta1, false, false, __LINE__ );

    checkBroadcast( "",
                    broadcastName,
                    { { "name", expressionName },
                      { Resources::CheckBroadcast::Has, "code" },
                      { Resources::CheckBroadcast::Has, "valueType" },
                      { Resources::CheckBroadcast::HasNot, "metadata" },
                      { Resources::CheckBroadcast::HasNot, "variable" } },
                    __LINE__ );

    scenesExpressionSetExt( expressionName, code, {}, {}, true, "float", meta2, false, false, __LINE__ );

    checkBroadcast( "",
                    broadcastName,
                    { { "name", expressionName },
                      { Resources::CheckBroadcast::Has, "code" },
                      { Resources::CheckBroadcast::HasNot, "valueType" },
                      { Resources::CheckBroadcast::Has, "metadata" },
                      { Resources::CheckBroadcast::HasNot, "variable" } },
                    __LINE__ );

    scenesExpressionSetExt( expressionName, code, {}, {}, true, "float", meta3, false, false, __LINE__ );

    checkBroadcast( "",
                    broadcastName,
                    { { "name", expressionName },
                      { Resources::CheckBroadcast::Has, "code" },
                      { Resources::CheckBroadcast::HasNot, "valueType" },
                      { Resources::CheckBroadcast::Has, "metadata" },
                      { Resources::CheckBroadcast::HasNot, "variable" } },
                    __LINE__ );

    scenesExpressionSetExt( expressionName, code, {}, {}, true, "bool", meta3, false, false, __LINE__ );

    checkBroadcast( "",
                    broadcastName,
                    { { "name", expressionName },
                      { Resources::CheckBroadcast::Has, "code" },
                      { Resources::CheckBroadcast::Has, "valueType" },
                      { Resources::CheckBroadcast::HasNot, "metadata" },
                      { Resources::CheckBroadcast::HasNot, "variable" } },
                    __LINE__ );

    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, set_updateTypedWithError_T1_2_2 ) /* 1.2.2 */ {
    const auto expressionName = "updateTypedWithError";
    auto code = "return 5 + 5";
    bool expressionExists = false;
    bool expectedBroadcast = true;
    std::string valueType = "";
    Json metadata = JSON_EMPTY_OBJ;
    scenesExpressionSetExt(
        expressionName, code, {}, {}, expressionExists, valueType, metadata, false, expectedBroadcast, __LINE__ );

    Json answer, expressions;
    runExpressionsList( answer, false );
    checkNoError( answer, __FILE__, __LINE__ );

    checkExpressions( answer, expressions, __LINE__ );
    Log::debug( "expressions, added: " + cc::j( expressions ) );
    checkExpressionByName( expressions, expressionName, {}, __LINE__ );

    {  // type mismatch
        auto newCode = "return 12 + 12";
        auto command = makeExpressionSetCommand( expressionName, newCode, {}, {}, "liter", "", false, false );

        auto answer = nma().sendtoHubAndGetAnswer( command );
        checkHasError( answer, __FILE__, __LINE__ );
    }
    {  // syntax error
        auto newCode = "return 12 + 12 syntax error here";
        auto command = makeExpressionSetCommand( expressionName, newCode, {}, {}, "liter", "", false, false );

        auto answer = nma().sendtoHubAndGetAnswer( command );
        checkHasError( answer, __FILE__, __LINE__ );

        runExpressionsList( answer, false );
        checkNoError( answer, __FILE__, __LINE__ );

        // original expression, unchanged
        checkExpressions( answer, expressions, __LINE__ );
        checkExpressionByName( expressions, expressionName, { { "code", code } }, __LINE__ );
    }

    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, expressionSet_createWithErrors_T1_3 ) /* 1.3 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const auto expressionName = test_info_->name();

    const char* code = "return 13 + 13 * 13 syntax-error-here";
    auto command = makeExpressionSetCommand( expressionName, code, {}, {}, "temperature", "", true, false );

    auto answer = nma().sendtoHubAndGetAnswer( command );
    checkHasError( answer, __FILE__, __LINE__ );
}

TEST_F( ScenesExpressionsTests, expressionSet_createWithParamsNoItemFound_T1_4 ) /* 1.4 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const auto expressionName = test_info_->name();
    const char* code = "return 14 + 14 * params.dimmer";
    JsonArray expressionItems = JSON_ARRAY_( JSON( "name"
                                                   << "dimmer"
                                                   << "_id"
                                                   << "doesNotExist" ) );
    auto command = makeExpressionSetCommand( expressionName, code, {}, expressionItems, "int", "", true, false );

    auto answer = nma().sendtoHubAndGetAnswer( command );
    checkHasError( answer, __FILE__, __LINE__ );
}

TEST_F( ScenesExpressionsTests, expressionSet_createWithParamsNoDeviceFound_T1_5 ) /* 1.5 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const auto expressionName = "NoDeviceFound";

    const char* code = "return 15 + 15 * params.dimmer";
    auto deviceItemClause = JSON( "name"
                                  << "siren"
                                  << "deviceName"
                                  << "Controller Siren"
                                  << "itemName"
                                  << "sound_volume" );

    const bool isVariable = false;
    auto command = makeExpressionSetCommand(
        expressionName, code, JSON_ARRAY_( deviceItemClause ), {}, "int", "", isVariable, false );
    auto answer = nma().sendtoHubAndGetAnswer( command );
    checkHasError( answer, __FILE__, __LINE__ );
    Json errorData;
    EXPECT_TRUE( json_get( answer, { "error", "data" }, errorData ) ) << "Should end with error";
    EXPECT_EQ( json_get_str( errorData ), "ezlo.device.notfound" );
}

TEST_F( ScenesExpressionsTests, expressionSet_createWithDeviceItemNames_T1_6 ) /* 1.6 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const auto expressionName = "withDeviceItems";

    // create device (siren), items: [sound_volume, sound_select]
    auto deviceId = makeDeviceSiren( __LINE__ );

    using namespace CoreD::Model;
    const std::string itemName = "sound_volume";
    const int value = 16;

    Item item = makeIntItem( deviceId, itemName, value );
    auto itemId = addItem( m_remote, item, __LINE__, nullptr );

    const char* code = "return params.siren * 4";
    auto deviceItemClause = JSON( "name"
                                  << "siren"
                                  << "deviceName"
                                  << "Controller Siren"
                                  << "itemName" << itemName );
    scenesExpressionSet( expressionName, code, JSON_ARRAY_( deviceItemClause ), {}, false, __LINE__ );

    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, expressionSet_createWithParamsItems_T1_7 ) /* 1.7 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const auto expressionName = "withItems";

    // create device (siren), items: [sound_volume, sound_select]
    auto deviceId = makeDeviceSiren( __LINE__ );

    using namespace CoreD::Model;
    const std::string itemName = "sound_volume";
    const int value = 17;
    Item item = makeIntItem( deviceId, itemName, value );
    auto itemId = addItem( m_remote, item, __LINE__, nullptr );

    const char* code = "return params.sound_volume_quad *4";
    auto itemsClause = JSON( "name"
                             << "sound_volume_quad"
                             << "_id" << itemId );
    scenesExpressionSet( expressionName, code, {}, JSON_ARRAY_( itemsClause ), false, __LINE__ );
    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, expressionSet_createScalableWithParamsItems_T1_8 ) /* 1.8 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const auto expressionName = "scalableWithItems";
    setDefaultTemperatureScaleSetting( CoreD::Model::ValueScales::Fahrenheit, __LINE__ );

    // create device (siren), items: [sound_volume, sound_select]
    auto deviceId = makeDeviceThermostat( __LINE__ );

    using namespace HubPlatform::CoreD;
    const std::string scale = Model::ValueScales::Celsius;
    double valueInScale = 0;

    const std::string itemName = "thermostat_setpoint";
    CoreD::Model::Item setPointItem = makeTemperatureItem( deviceId, itemName, valueInScale, scale.c_str() );

    auto itemId = addItem( m_remote, setPointItem, __LINE__, nullptr );

    auto code = "return params." + itemName;
    auto itemClause = JSON( "name" << itemName << "_id" << itemId );
    scenesExpressionSet( expressionName, code, {}, JSON_ARRAY_( itemClause ), false, __LINE__ );

    Json answer;
    JsonArray expressions;
    runExpressionsList( answer, false );
    checkNoError( answer, __FILE__, __LINE__ );

    /* [{"name":"scalableWithItems","params":{"items":[{"_id":"610bfe5900000022c81b4bec","name":"temperature_setpoint"}]},"value":{"scale":"celsius","value":0}}]
     */
    checkExpressions( answer, expressions, __LINE__ );
    Log::debug( "expressions: " + cc::j( expressions ) );
    // TODO: fetch value and check it is an object { value: number, scale: string }
    checkExpressionByName( expressions,
                           expressionName,
                           { { "value", Json{ { "value", valueInScale }, { "scale", "celsius" } } } },
                           __LINE__ );

    // change item value
    float updatedValueF = toFahrenheit( valueInScale + 100 );
    setItemValue( m_remote, itemId, updatedValueF, __LINE__, CoreD::Model::ValueScales::Fahrenheit );

    runExpressionsList( answer, false );
    checkNoError( answer, __FILE__, __LINE__ );

    checkExpressions( answer, expressions, __LINE__ );
    Log::debug( "expressions, updated: " + cc::j( expressions ) );
    checkExpressionByName(
        expressions,
        expressionName,
        { { "value", Json{ { "value", updatedValueF }, { "scale", Model::ValueScales::Fahrenheit } } } },
        __LINE__ );
    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, expressionSet_createWithWrongType_T1_9 ) /* 1.9 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const auto expressionName = "createWithWrongType";
    const char* code = "return 2 + 2 * 2";
    Json meta = { { "info", "non-existent type passed" } };
    Json answer;

    auto command = makeExpressionSetCommand( expressionName, code, {}, {}, "non_existent_type", meta, false, false );
    answer = nma().sendtoHubAndGetAnswer( command );
    checkHasError( answer, __FILE__, __LINE__ );

    Json errorData;
    EXPECT_TRUE( json_get( answer, { "error", "data" }, errorData ) );
    EXPECT_EQ( json_get_str( errorData ), "scenes.item.value.type.error" ) << "Should return item value type error";
}

TEST_F( ScenesExpressionsTests, expressionSet_createWithTypeMismatch_T1_9_1 ) /* 1.9.1 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const auto expressionName = "createWithTypeMismatch";
    const char* code = "return 2 + 2 * 2";
    Json meta = { { "info", "Format of data doesn't correspond to valueType" } };
    Json answer;

    auto command = makeExpressionSetCommand( expressionName, code, {}, {}, "rgb", "", false, false );
    answer = nma().sendtoHubAndGetAnswer( command );
    checkHasError( answer, __FILE__, __LINE__ );

    Json errorData;
    EXPECT_TRUE( json_get( answer, { "error", "data" }, errorData ) );
    EXPECT_EQ( json_get_str( errorData ), "scenes.expression.type.mismatch" ) << "Should return type mismatch error";
}

TEST_F( ScenesExpressionsTests, expressionSet_createWithParamsFalseVariable_T1_10 ) /* 1.10 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const std::string expressionName = "expressionSetFalseVariable";
    auto deviceId = makeDeviceSiren( __LINE__ );
    using namespace CoreD::Model;
    auto variable = false;
    JsonArray names = JSON_EMPTY_ARR;
    bool showValue = true;
    Item item1 = makeIntItem( deviceId, "item", 5 );
    auto expressionItemId = addItem( m_remote, item1, __LINE__, nullptr );
    auto code = "return 2 + 2 * 2";
    auto itemClause = JSON( "name"
                            << "expression_item"
                            << "_id" << expressionItemId );
    scenesExpressionSetExt(
        expressionName, code, {}, JSON_ARRAY_( itemClause ), false, "", {}, variable, true, __LINE__ );
    Json answer;
    runExpressionsList( answer, true );
    checkNoError( answer, __FILE__, __LINE__ );
    JsonArray expressions;
    checkExpressions( answer, expressions, __LINE__ );
    runExpressionsListWithFilter( answer, true, true, { "" }, false, names, showValue );
    checkNoError( answer, __FILE__, __LINE__ );
    checkExpressions( answer, expressions, __LINE__ );
    checkExpressionByName( expressions,
                           expressionName,
                           {
                               { Resources::CheckBroadcast::HasNot, "error" },
                               { Resources::CheckBroadcast::Has, "value" },
                           },
                           __LINE__ );
    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, setExpressionActionBlock_T1_11 ) /* 1.11 */ {
    Log::setLocalLogLevel( Log::TRACE );
    const auto expressionName = "setExpressionAction";
    auto code = "return 1 + 2 * 2";

    auto deviceId = makeDeviceSiren( __LINE__ );

    using namespace CoreD::Model;
    const std::string itemName = "sound_volume";
    const int value = 411;

    CoreD::Model::Item item = makeIntItem( deviceId, itemName, value );
    auto itemId = addItem( m_remote, item, __LINE__, nullptr );

    auto itemClause = JSON( "name"
                            << "sound_volume_to_ten"
                            << "_id" << itemId );

    scenesExpressionSet( expressionName, code, {}, JSON_ARRAY_( itemClause ), false, __LINE__ );
    auto request = replaceTokensInString( createSceneRequest,
                                          { { "__NAME__", "setExpression" },
                                            { "__WHEN_BLOCKS__", "" },
                                            { "__THEN_BLOCKS__", HubPlatform::Tests::setExpression } } );

    JsonObject setExpressionObj = JsonObject::parse( request );
    SceneID sceneId = createScene( setExpressionObj, __LINE__ );
    runSceneManually( sceneId, __LINE__ );

    testRunProgressBroadcast( setExpressionObj, sceneId, "started", __LINE__ );
    checkBroadcast( "", "hub.expression.changed", {}, __LINE__ );
    testRunProgressBroadcast( setExpressionObj, sceneId, "finished", __LINE__ );

    Json answer;
    JsonArray expressions;
    runExpressionsList( answer, true );
    checkNoError( answer, __FILE__, __LINE__ );
    LOGGER_TRACE( cc::trace( "expressions list: " ) + cc::j( answer ) );
    checkExpressions( answer, expressions, __LINE__ );

    deleteScene( sceneId );
    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, setExpressionActionBlockWithParam_T1_12 ) /* 1.12 */ {
    Log::setLocalLogLevel( Log::TRACE );
    const auto expressionName = "setExprAction";
    auto code = "return 1 + 2 * 2";

    auto deviceId = makeDeviceSiren( __LINE__ );

    using namespace CoreD::Model;
    const std::string itemName = "sound_volume";
    const int value = 411;

    CoreD::Model::Item item = makeIntItem( deviceId, itemName, value );
    auto itemId = addItem( m_remote, item, __LINE__, nullptr );

    auto itemClause = JSON( "name"
                            << "sound_volume_to_ten"
                            << "_id" << itemId );

    scenesExpressionSet( expressionName, code, {}, {}, false, __LINE__ );

    {
        Json answer;
        JsonArray expressions;
        runExpressionsList( answer, true );
        checkNoError( answer, __FILE__, __LINE__ );
        LOGGER_TRACE( cc::trace( "! expressions list: " ) + cc::j( answer ) );
        checkExpressions( answer, expressions, __LINE__ );
        auto item1 = JSON( "_id" << itemId << "name"
                                 << "sound_volume_to_ten" );
        auto params = JSON( "items" << JSON_ARRAY( item1 ) );

        checkExpressionByName( expressions, expressionName, {}, __LINE__ );
    }

    auto setExpressionBlock =
        replaceTokensInString( HubPlatform::Tests::setExpressionWithParams, { { "__ID__", itemId } } );

    auto request = replaceTokensInString(
        createSceneRequest,
        { { "__NAME__", "setExpression" }, { "__WHEN_BLOCKS__", "" }, { "__THEN_BLOCKS__", setExpressionBlock } } );

    JsonObject setExpressionObj = JsonObject::parse( request );
    SceneID sceneId = createScene( setExpressionObj, __LINE__ );
    runSceneManually( sceneId, __LINE__ );

    testRunProgressBroadcast( setExpressionObj, sceneId, "started", __LINE__ );
    checkBroadcast( "", "hub.expression.changed", {}, __LINE__ );
    testRunProgressBroadcast( setExpressionObj, sceneId, "finished", __LINE__ );

    Json answer;
    JsonArray expressions;
    runExpressionsList( answer, true );
    checkNoError( answer, __FILE__, __LINE__ );
    LOGGER_TRACE( cc::trace( "expressions list: " ) + cc::j( answer ) );
    checkExpressions( answer, expressions, __LINE__ );
    auto item1 = JSON( "_id" << itemId << "name"
                             << "sound_volume_to_ten" );
    auto params = JSON( "items" << JSON_ARRAY( item1 ) );

    checkExpressionByName( expressions, expressionName, { { "params", params } }, __LINE__ );

    deleteScene( sceneId );
    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, setExpressionValue_T1_13 ) /* 1.13 */ {
    Log::setLocalLogLevel( Log::TRACE );
    const auto expressionName = "variableExpression";
    auto deviceId = makeDeviceSiren( __LINE__ );

    using namespace CoreD::Model;
    const std::string itemName = "sound_volume";
    const int value = 411;

    CoreD::Model::Item item = makeIntItem( deviceId, itemName, value );
    auto itemId = addItem( m_remote, item, __LINE__, nullptr );

    bool expressionValue = false;
    bool isExpectedBroadcast = true;
    bool variable = true;
    scenesVariableExpressionSetType< int >(
        expressionName, expressionValue, "hub.expression.added", "bool", {}, isExpectedBroadcast, __LINE__ );

    auto setExpressionBlock =
        replaceTokensInString( HubPlatform::Tests::setValueActonBlock, { { "__NAME__", "variableExpression" } } );

    auto request = replaceTokensInString( createSceneRequest,
                                          { { "__NAME__", "setExpressionValue" },
                                            { "__WHEN_BLOCKS__", "" },
                                            { "__THEN_BLOCKS__", setExpressionBlock } } );

    JsonObject setExpressionObj = JsonObject::parse( request );
    SceneID sceneId = createScene( setExpressionObj, __LINE__ );
    runSceneManually( sceneId, __LINE__ );

    testRunProgressBroadcast( setExpressionObj, sceneId, "started", __LINE__ );
    checkBroadcast( "", "hub.expression.changed", {}, __LINE__ );
    testRunProgressBroadcast( setExpressionObj, sceneId, "finished", __LINE__ );
    Json answer;
    JsonArray expressions;
    runExpressionsList( answer, false );
    checkNoError( answer, __FILE__, __LINE__ );
    LOGGER_TRACE( cc::trace( "expressions list: " ) + cc::j( answer ) );
    checkExpressions( answer, expressions, __LINE__ );
    auto metadata = JSON( "key"
                          << "value" );
    checkExpressionByName( expressions,
                           expressionName,
                           { { "value", 123 },
                             { "metadata", metadata },
                             { "valueType", "int" },
                             { "params", JSON_EMPTY_OBJ },
                             { Resources::CheckBroadcast::HasNot, "code" } },
                           __LINE__ );

    deleteScene( sceneId );

    setExpressionBlock = replaceTokensInString( HubPlatform::Tests::setValueActonBlock, { { "__NAME__", "expr" } } );

    request = replaceTokensInString( createSceneRequest,
                                     { { "__NAME__", "setExpressionValue" },
                                       { "__WHEN_BLOCKS__", "" },
                                       { "__THEN_BLOCKS__", HubPlatform::Tests::setValueActonBlock } } );

    answer = nma().sendtoHubAndGetAnswer( request );
    checkHasError( answer, __FILE__, __LINE__ );
    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, createExpressionWithErrorNotFoundItem ) /* 1.14 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const auto expressionName = "withDeviceItems";

    // create device (siren), items: [sound_volume, sound_select]
    auto deviceId = makeDeviceSiren( __LINE__ );

    using namespace CoreD::Model;
    const std::string itemName = "sound_volume";
    const int value = 16;

    Item item = makeIntItem( deviceId, itemName, value );
    auto itemId = addItem( m_remote, item, __LINE__, nullptr );

    const char* code = "return params.siren * 4";
    auto itemClause = JSON( "name"
                            << "sound_volume_to_ten"
                            << "_id"
                            << "itemId" );
    Json command = makeExpressionSetCommand(
        expressionName, code, {}, JSON_ARRAY_( itemClause ), "", JSON_EMPTY_OBJ, false, true );

    auto answer = nma().sendtoHubAndGetAnswer( command );

    LOGGER_ERROR( "answer: " + cc::j( answer ) );

    checkHasError( answer, __FILE__, __LINE__ );

    Json errorData;
    EXPECT_TRUE( json_get( answer, { "error", "data" }, errorData ) );
    EXPECT_EQ( json_get_str( errorData ), "ezlo.item.notfound" );
}

TEST_F( ScenesExpressionsTests, createExpressionWitherrorDevice ) {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const auto expressionName = "withDeviceItems"; /* 1.15 */

    // create device (siren), items: [sound_volume, sound_select]
    auto deviceId = makeDeviceSiren( __LINE__ );

    using namespace CoreD::Model;
    const std::string itemName = "sound_volume";
    const int value = 16;

    Item item = makeIntItem( deviceId, itemName, value );
    auto itemId = addItem( m_remote, item, __LINE__, nullptr );

    const char* code = "return params.siren * 4";
    auto deviceItemClause = JSON( "name"
                                  << "siren"
                                  << "deviceName"
                                  << "Controller Siren1"
                                  << "itemName" << itemName );

    Json command = makeExpressionSetCommand(
        expressionName, code, JSON_ARRAY_( deviceItemClause ), {}, "", JSON_EMPTY_OBJ, false, true );

    auto answer = nma().sendtoHubAndGetAnswer( command );
    checkHasError( answer, __FILE__, __LINE__ );

    Json errorData;
    EXPECT_TRUE( json_get( answer, { "error", "data" }, errorData ) );
    EXPECT_EQ( json_get_str( errorData ), "ezlo.device.notfound" );
}

TEST_F( ScenesExpressionsTests, expressionCreateToLongExpressionName ) /* 1.16 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const auto expressionName = test_info_->name();
    const auto code = "return 2 + 2 * 2";
    Json command = makeExpressionSetCommand( expressionName, code, {}, {}, "", JSON_EMPTY_OBJ, false, true );

    auto answer = nma().sendtoHubAndGetAnswer( command );
    checkHasError( answer, __FILE__, __LINE__ );

    Json errorData;
    EXPECT_TRUE( json_get( answer, { "error", "data" }, errorData ) );
    EXPECT_EQ( json_get_str( errorData ), "ezlo.value.too.long" );
}

TEST_F( ScenesExpressionsTests, expressionCreateTypeError ) /* 1.17 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const auto expressionName = test_info_->name();

    const char* code = "return 13 + 13 * 13";
    auto command = makeExpressionSetCommand( expressionName, code, {}, {}, "temperature", "", false, false );

    auto answer = nma().sendtoHubAndGetAnswer( command );
    checkHasError( answer, __FILE__, __LINE__ );
    Json errorData;
    EXPECT_TRUE( json_get( answer, { "error", "data" }, errorData ) );
    EXPECT_EQ( json_get_str( errorData ), "scenes.expression.type.mismatch" );
}

TEST_F( ScenesExpressionsTests, expressionSet_delete_T2_1 ) /* 2.1 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const auto expressionName = test_info_->name();
    auto code = "return 2 + 2 * 2";

    scenesExpressionSet( expressionName, code, {}, {}, false, __LINE__ );

    auto command = PARSE_JSON( SceneCommands::expressionDelete );
    command[ "params" ][ "name" ] = expressionName;
    Json answer = nma().sendtoHubAndGetAnswer( command );
    checkNoError( answer, __FILE__, __LINE__ );
    checkBroadcast( "", "hub.expression.deleted", {}, __LINE__ );
}

TEST_F( ScenesExpressionsTests, expressionSet_deleteMissing_T2_2 ) /* 2.2 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    Json command = PARSE_JSON( SceneCommands::expressionDelete );
    command[ "params" ][ "name" ] = "DoesNotExist";

    auto answer = nma().sendtoHubAndGetAnswer( command );
    checkHasError( answer, __FILE__, __LINE__ );
}

TEST_F( ScenesExpressionsTests, expressionSet_list_T3_1 ) /* 3.1 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const auto expressionName = "list";
    const char* code = "return 2 + 2 * 2";

    scenesExpressionSet( expressionName, code, {}, {}, false, __LINE__ );

    Json answer;
    runExpressionsList( answer, true );
    checkNoError( answer, __FILE__, __LINE__ );

    JsonArray expressions;
    checkExpressions( answer, expressions, __LINE__ );

    // array elements should have { name: string, params?: {}, code: string?, value: any } fields

    ASSERT_GE( expressions.size(), 1 );
    const auto& single = expressions.at( 0 );

    LOGGER_ERROR( "expressions: " + cc::j( expressions ) );

    checkExpressionByName( expressions,
                           expressionName,
                           {
                               { Resources::CheckBroadcast::HasNot, "error" },
                               { Resources::CheckBroadcast::Has, "value" },
                               { Resources::CheckBroadcast::Has, "code" },
                           },
                           __LINE__ );

    // optional: params
    Json params;
    if( json_get( single, "params", params ) ) {
        EXPECT_TRUE( params.is_object() );
    }
    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, expressionSet_listWithCode_T3_2 ) /* 3.2 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const auto expressionName = "listWithCode";
    const char* code = "return 2 + 2 * 2";
    scenesExpressionSet( expressionName, code, {}, {}, false, __LINE__ );

    Json answer;
    runExpressionsList( answer, true );
    checkNoError( answer, __FILE__, __LINE__ );

    JsonArray expressions;
    checkExpressions( answer, expressions, __LINE__ );

    // array elements should have { name: string, params?: {}, code: string?, value: any } fields
    ASSERT_GE( expressions.size(), 1 );
    const auto& single = expressions.at( 0 );
    EXPECT_TRUE( single.count( "name" ) );
    EXPECT_TRUE( single.count( "value" ) );
    EXPECT_TRUE( single.count( "code" ) );  // should exist if showCode = true, should not if
    // optional: params
    Json params;
    if( json_get( single, "params", params ) ) {
        EXPECT_TRUE( params.is_object() );
    }
    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, expressionSet_listWithCodeValueTypeMetaVariable_T3_3_1 ) /* 3.3.1 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const auto expressionName = "listWithCodeValueTypeMetaVar";
    const char* code = "return 2 + 2 * 2";
    Json metadata = JSON( "meta"
                          << "123" );
    bool isVariable = false;
    std::string valueType = "int";
    scenesExpressionSetExt( expressionName, code, {}, {}, false, valueType, metadata, isVariable, true, __LINE__ );

    Json answer;
    runExpressionsList( answer, true );
    checkNoError( answer, __FILE__, __LINE__ );

    JsonArray expressions;
    checkExpressions( answer, expressions, __LINE__ );

    Json single;
    ASSERT_GE( expressions.size(), 1 );
    //    single = expressions.at( 0 );
    checkExpressionByName( expressions,
                           expressionName,
                           { { Resources::CheckBroadcast::HasNot, "error" },
                             { Resources::CheckBroadcast::Has, "value" },
                             { "valueType", valueType },
                             { "metadata", metadata },
                             { "variable", isVariable } },
                           __LINE__ );

    // Modify expression: variable
    const auto broadcastName = "hub.expression.changed";
    const bool expressionExists = true;

    {
        // modify valueType field, check broadcast, should have only required fields (name, code) and changed field
        valueType = "float";
        scenesExpressionSetExt(
            expressionName, code, {}, {}, expressionExists, valueType, metadata, isVariable, false, __LINE__ );

        checkBroadcast( "",
                        broadcastName,
                        { { "name", expressionName },
                          { Resources::CheckBroadcast::Has, "code" },
                          { "valueType", valueType },
                          { Resources::CheckBroadcast::HasNot, "metadata" },
                          { Resources::CheckBroadcast::HasNot, "variable" } },
                        __LINE__ );
    }
    {
        // modify metadata field, check broadcast, should have only required fields (name, code) and changed field
        metadata = { { "info", "changed metadata" } };
        scenesExpressionSetExt(
            expressionName, code, {}, {}, expressionExists, valueType, metadata, isVariable, false, __LINE__ );

        checkBroadcast( "",
                        broadcastName,
                        { { "name", expressionName },
                          { Resources::CheckBroadcast::Has, "code" },
                          { Resources::CheckBroadcast::HasNot, "valueType" },
                          { "metadata", metadata },
                          { Resources::CheckBroadcast::HasNot, "variable" } },
                        __LINE__ );
    }

    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, expressionSet_listWithFilters_T3_3_2 ) /* 3.3.2 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const auto expressionName = "listWithFilters";
    const char* code = "return 2 + 2 * 2";
    Json meta = JSON( "meta"
                      << "123" );
    Json answer;
    bool expectBroadcast = true;
    JsonArray names = JSON_EMPTY_ARR;
    bool showValue = false;

    scenesExpressionSetExt( expressionName, code, {}, {}, false, "int", meta, false, expectBroadcast, __LINE__ );
    runExpressionsListWithFilter( answer, true, true, { "int" }, false, names, showValue );
    checkNoError( answer, __FILE__, __LINE__ );
    JsonArray expressions;
    checkExpressions( answer, expressions, __LINE__ );
    ASSERT_EQ( expressions.size(), 1 );
    runExpressionsListWithFilter( answer, true, true, { "string" }, false, names, showValue );
    checkNoError( answer, __FILE__, __LINE__ );
    checkExpressions( answer, expressions, __LINE__ );
    ASSERT_EQ( expressions.size(), 0 );
    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, expressionSet_listWithFilters_T3_3_3 ) /* 3.3.3 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );

    const auto expressionName1 = "listWithFilters_T3_expr1";
    const auto expressionName2 = "listWithFilters_T3_expr2";
    const auto expressionName3 = "listWithFilters_T3_expr3";
    const auto expressionName4 = "listWithFilters_T3_expr4";
    const auto expressionName5 = "listWithFilters_T3_expr5";
    const auto expressionName6 = "listWithFilters_T3_expr6";

    Json answer;
    JsonArray expressions;
    JsonArray names = JSON_EMPTY_ARR;
    bool showValue = false;
    const char* code = "return 2 + 2 * 2";
    Json meta = JSON( "meta"
                      << "123" );
    scenesExpressionSetExt( expressionName1, code, {}, {}, false, "int", meta, false, true, __LINE__ );
    runExpressionsListWithFilter( answer, true, true, { "int" }, false, names, showValue );
    checkNoError( answer, __FILE__, __LINE__ );
    checkExpressions( answer, expressions, __LINE__ );
    ASSERT_GE( expressions.size(), 1 );
    runExpressionsListWithFilter( answer, true, true, { "string" }, true, names, showValue );
    checkNoError( answer, __FILE__, __LINE__ );
    checkExpressions( answer, expressions, __LINE__ );
    ASSERT_EQ( expressions.size(), 0 );
    scenesExpressionSetExt( expressionName2, code, {}, {}, false, "bool", meta, false, true, __LINE__ );
    scenesExpressionSet( expressionName4, code, {}, {}, false, __LINE__ );
    scenesExpressionSet( expressionName5, code, {}, {}, false, __LINE__ );
    runExpressionsListWithFilter( answer, true, true, { "bool" }, false, names, showValue );
    checkNoError( answer, __FILE__, __LINE__ );
    checkExpressions( answer, expressions, __LINE__ );
    ASSERT_GE( expressions.size(), 1 ) << "Should be one boolean expression";
    scenesExpressionSet( expressionName6, code, {}, {}, false, __LINE__ );
    scenesExpressionSetExt( expressionName3, code, {}, {}, false, "", meta, false, true, __LINE__ );
    runExpressionsListWithFilter( answer, true, true, { "string" }, false, names, showValue );
    checkNoError( answer, __FILE__, __LINE__ );
    checkExpressions( answer, expressions, __LINE__ );
    ASSERT_EQ( expressions.size(), 0 );
    runExpressionsListWithFilter( answer, true, true, {}, false, names, showValue );
    checkNoError( answer, __FILE__, __LINE__ );
    checkExpressions( answer, expressions, __LINE__ );
    ASSERT_GE( expressions.size(), 3 );
    deleteSceneExpression( expressionName1, __LINE__ );
    deleteSceneExpression( expressionName2, __LINE__ );
    deleteSceneExpression( expressionName3, __LINE__ );
    deleteSceneExpression( expressionName4, __LINE__ );
    deleteSceneExpression( expressionName5, __LINE__ );
    deleteSceneExpression( expressionName6, __LINE__ );
}

TEST_F( ScenesExpressionsTests, expressionListWithFilteredNames_T3_4_1 ) /* 3.4.1 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const auto expressionName1 = "expr1";
    const auto expressionName2 = "expr2";
    const auto expressionName3 = "expr3";
    const auto expressionName4 = "expr4";
    const char* code = "return 2 + 2 * 2";
    Json meta = JSON( "meta"
                      << "123" );
    Json answer;
    bool expectBroadcast = true;
    JsonArray names = { expressionName1, expressionName3 };
    bool showValue = false;

    scenesExpressionSetExt( expressionName1, code, {}, {}, false, "int", meta, false, expectBroadcast, __LINE__ );
    scenesExpressionSetExt( expressionName2, code, {}, {}, false, "int", meta, false, expectBroadcast, __LINE__ );
    scenesExpressionSetExt( expressionName3, code, {}, {}, false, "int", meta, false, expectBroadcast, __LINE__ );
    scenesExpressionSetExt( expressionName4, code, {}, {}, false, "int", meta, false, expectBroadcast, __LINE__ );

    runExpressionsListWithFilter( answer, true, true, { "int" }, false, names, showValue );
    checkNoError( answer, __FILE__, __LINE__ );
    JsonArray expressions;
    checkExpressions( answer, expressions, __LINE__ );
    ASSERT_EQ( expressions.size(), 2 );

    checkExpressionByName( expressions, expressionName1, {}, __LINE__ );
    checkExpressionByName( expressions, expressionName3, {}, __LINE__ );

    runExpressionsList( answer, true );
    checkExpressions( answer, expressions, __LINE__ );
    ASSERT_EQ( expressions.size(), 4 );

    checkExpressionByName( expressions, expressionName1, {}, __LINE__ );
    checkExpressionByName( expressions, expressionName2, {}, __LINE__ );
    checkExpressionByName( expressions, expressionName3, {}, __LINE__ );
    checkExpressionByName( expressions, expressionName4, {}, __LINE__ );

    names = JSON_EMPTY_ARR;
    runExpressionsListWithFilter( answer, true, true, { "int" }, false, names, showValue );
    checkExpressions( answer, expressions, __LINE__ );
    ASSERT_EQ( expressions.size(), 4 );

    checkExpressionByName( expressions, expressionName1, {}, __LINE__ );
    checkExpressionByName( expressions, expressionName2, {}, __LINE__ );
    checkExpressionByName( expressions, expressionName3, {}, __LINE__ );
    checkExpressionByName( expressions, expressionName4, {}, __LINE__ );

    deleteSceneExpression( expressionName1, __LINE__ );
    deleteSceneExpression( expressionName2, __LINE__ );
    deleteSceneExpression( expressionName3, __LINE__ );
    deleteSceneExpression( expressionName4, __LINE__ );
}

TEST_F( ScenesExpressionsTests, expressionListWithShowValueFilter_T3_4_2 ) /* 3.4.2 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const std::string expressionName = "expression";
    auto deviceId = makeDeviceSiren( __LINE__ );
    using namespace CoreD::Model;
    auto variable = false;
    Item item1 = makeIntItem( deviceId, "item", 5 );
    auto expressionItemId = addItem( m_remote, item1, __LINE__, nullptr );
    auto code = "return 2 + 2 * 2";
    auto itemClause = JSON( "name"
                            << "expression_item"
                            << "_id" << expressionItemId );
    scenesExpressionSetExt(
        expressionName, code, {}, JSON_ARRAY_( itemClause ), false, "", {}, variable, true, __LINE__ );
    Json answer;
    runExpressionsList( answer, true );
    checkNoError( answer, __FILE__, __LINE__ );
    JsonArray expressions;
    checkExpressions( answer, expressions, __LINE__ );
    checkExpressionByName( expressions,
                           expressionName,
                           {
                               { Resources::CheckBroadcast::HasNot, "error" },
                               { Resources::CheckBroadcast::Has, "value" },
                           },
                           __LINE__ );
    JsonArray names = JSON_EMPTY_ARR;
    bool showValue = true;
    runExpressionsListWithFilter( answer, true, true, { "" }, false, names, showValue );
    checkNoError( answer, __FILE__, __LINE__ );
    checkExpressions( answer, expressions, __LINE__ );
    checkExpressionByName( expressions,
                           expressionName,
                           {
                               { Resources::CheckBroadcast::HasNot, "error" },
                               { Resources::CheckBroadcast::Has, "value" },
                           },
                           __LINE__ );

    showValue = false;
    runExpressionsListWithFilter( answer, true, true, { "" }, false, names, showValue );
    checkNoError( answer, __FILE__, __LINE__ );
    checkExpressions( answer, expressions, __LINE__ );
    checkExpressionByName( expressions,
                           expressionName,
                           {
                               { Resources::CheckBroadcast::HasNot, "error" },
                               { Resources::CheckBroadcast::HasNot, "value" },
                           },
                           __LINE__ );

    deleteSceneExpression( expressionName, __LINE__ );
}

namespace Resources {
static const char* HUB_EXPRESSION_ADDED = "hub.expression.added";
}

TEST_F( ScenesExpressionsTests, expressionSetWithParamsTrueVariable_T3_3_6 ) /* 3.3.6 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const std::string expressionName = "expressionSetTrueVariable";

    auto deviceId = makeDeviceSiren( __LINE__ );
    using namespace CoreD::Model;
    Item item1 = makeIntItem( deviceId, "itemName", 5 );
    auto expressionItemId = addItem( m_remote, item1, __LINE__, nullptr );
    auto code = "return 2 + 2 * 2";
    auto variable = false;
    auto itemClause = JSON( "name"
                            << "expression_item"
                            << "_id" << expressionItemId );

    Json command =
        makeExpressionSetCommand( expressionName, code, {}, JSON_ARRAY_( itemClause ), "int", {}, variable, false );
    auto answer = nma().sendtoHubAndGetAnswer( command );
    checkNoError( answer, __FILE__, __LINE__ );
    checkBroadcast( "", Resources::HUB_EXPRESSION_ADDED, { { "name", expressionName } }, __LINE__ );

    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, eventTracking_itemValueUpdated_T4_1_1 ) /* 4.1.1 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const auto expressionName = "itemValueUpdated";

    // create device (siren), items: [sound_volume, sound_select]
    auto deviceId = makeDeviceSiren( __LINE__ );

    using namespace CoreD::Model;
    const std::string itemName = "sound_volume";
    const int value = 411;

    CoreD::Model::Item item = makeIntItem( deviceId, itemName, value );
    auto itemId = addItem( m_remote, item, __LINE__, nullptr );

    const char* code = "return params.sound_volume_to_ten * 10";
    auto itemClause = JSON( "name"
                            << "sound_volume_to_ten"
                            << "_id" << itemId );
    scenesExpressionSet( expressionName, code, {}, JSON_ARRAY_( itemClause ), false, __LINE__ );

    const auto updatedExpressionValue = ( value / 4 ) * 10;
    // emit item.updated event
    setItemValue( m_remote, itemId, value / 4, __LINE__ );

    // check output
    {
        Json answer;
        runExpressionsList( answer, true );
        checkNoError( answer, __FILE__, __LINE__ );

        JsonArray expressions;
        checkExpressions( answer, expressions, __LINE__ );
        checkExpressionByName( expressions,
                               expressionName,
                               { { "value", updatedExpressionValue }, { Resources::CheckBroadcast::HasNot, "error" } },
                               __LINE__ );
    }
    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, eventTracking_itemNameUpdated_T4_1_2 ) /* 4.1.2 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const auto expressionName = "itemNameUpdated";

    const auto deviceName = "Controller Siren";
    auto deviceId = makeDeviceSiren( __LINE__ );

    using namespace CoreD::Model;
    const std::string itemName = "sound_volume";
    const int value = 412;

    Item item = makeIntItem( deviceId, itemName, value );
    auto itemId = addItem( m_remote, item, __LINE__, nullptr );

    auto code = "return params.sound_volume_to_ten * 10";
    auto deviceItemClause = JSON( "name"
                                  << "sound_volume_to_ten"
                                  << "deviceName" << deviceName << "itemName" << itemName );
    scenesExpressionSet( expressionName, code, JSON_ARRAY_( deviceItemClause ), {}, false, __LINE__ );

    Json answer;
    // emit item.updated event with modified name field
    {
        Item updatedItem = makeIntItem( deviceId, "anotherItemName", value );
        m_remote.modifyItem( itemId, updatedItem );

        checkBroadcast( itemId, "hub.item.updated", {}, __LINE__ );
    }

    // should produce expression error
    {
        runExpressionsList( answer, false );
        checkNoError( answer, __FILE__, __LINE__ );

        JsonArray expressions;
        checkExpressions( answer, expressions, __LINE__ );
        checkExpressionByName(
            expressions,
            expressionName,
            { { Resources::CheckBroadcast::Has, "error" }, { Resources::CheckBroadcast::HasNot, "value" } },
            __LINE__ );
    }

    // emit item.updated event with original name field and modified value
    {
        Item updatedItem = makeIntItem( deviceId, itemName, value / 4 );
        m_remote.modifyItem( itemId, updatedItem );

        checkBroadcast( itemId, "hub.item.updated", {}, __LINE__ );
    }

    const auto updatedExpressionValue = ( value / 4 ) * 10;
    // check output
    {
        runExpressionsList( answer, true );
        checkNoError( answer, __FILE__, __LINE__ );

        JsonArray expressions;
        checkExpressions( answer, expressions, __LINE__ );
        checkExpressionByName( expressions,
                               expressionName,
                               { { "value", updatedExpressionValue }, { Resources::CheckBroadcast::HasNot, "error" } },
                               __LINE__ );
    }
    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, eventTracking_itemRemoved_T4_1_3 ) /* 4.1.3 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const auto expressionName = "itemRemoved";

    const auto deviceName = "Controller Siren";
    auto deviceId = makeDeviceSiren( __LINE__ );

    using namespace CoreD::Model;
    const std::string itemName = "sound_volume";
    const int value = 413;

    CoreD::Model::Item item = makeIntItem( deviceId, itemName, value );
    auto itemId = addItem( m_remote, item, __LINE__, nullptr );

    auto code = "return params.sound_volume_minus_13 - 13";
    auto deviceItemClause = JSON( "name"
                                  << "sound_volume_minus_13"
                                  << "deviceName" << deviceName << "itemName" << itemName );
    scenesExpressionSet( expressionName, code, JSON_ARRAY_( deviceItemClause ), {}, false, __LINE__ );

    const auto updatedExpressionValue = ( value - 13 );
    // check output
    Json answer;
    {
        runExpressionsList( answer, true );
        checkNoError( answer, __FILE__, __LINE__ );
        JsonArray expressions;
        checkExpressions( answer, expressions, __LINE__ );

        checkExpressionByName( expressions,
                               expressionName,
                               { { "value", updatedExpressionValue }, { Resources::CheckBroadcast::HasNot, "error" } },
                               __LINE__ );
    }

    removeItem( m_remote, itemId, __LINE__ );

    {
        runExpressionsList( answer, true );
        checkNoError( answer, __FILE__, __LINE__ );

        JsonArray expressions;
        checkExpressions( answer, expressions, __LINE__ );
        checkExpressionByName(
            expressions,
            expressionName,
            { { Resources::CheckBroadcast::HasNot, "value" }, { Resources::CheckBroadcast::Has, "error" } },
            __LINE__ );
    }
    deleteSceneExpression( expressionName, __LINE__ );
}

// Previously removed item added back
TEST_F( ScenesExpressionsTests, eventTracking_removedItemAddedBack_T4_1_4 ) /* 4.1.4 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const auto expressionName = "removedItemAddedBack";

    const auto deviceName = "Controller Siren";
    auto deviceId = makeDeviceSiren( __LINE__ );

    using namespace CoreD::Model;
    const std::string itemName = "sound_volume";
    const int value = 414;

    CoreD::Model::Item item = makeIntItem( deviceId, itemName, value );
    auto itemId = addItem( m_remote, item, __LINE__, nullptr );

    auto code = "return params.sound_volume_minus_414 - 414";
    auto deviceItemClause = JSON( "name"
                                  << "sound_volume_minus_414"
                                  << "deviceName" << deviceName << "itemName" << itemName );
    scenesExpressionSet( expressionName, code, JSON_ARRAY_( deviceItemClause ), {}, false, __LINE__ );

    Json answer;
    const auto updatedExpressionValue = ( value - 414 );
    // check output
    {
        runExpressionsList( answer, true );
        checkNoError( answer, __FILE__, __LINE__ );

        JsonArray expressions;
        checkExpressions( answer, expressions, __LINE__ );
        checkExpressionByName( expressions,
                               expressionName,
                               { { "value", updatedExpressionValue }, { Resources::CheckBroadcast::HasNot, "error" } },
                               __LINE__ );
    }

    removeItem( m_remote, itemId, __LINE__ );

    {
        runExpressionsList( answer, true );
        checkNoError( answer, __FILE__, __LINE__ );

        JsonArray expressions;
        checkExpressions( answer, expressions, __LINE__ );
        checkExpressionByName(
            expressions,
            expressionName,
            { { Resources::CheckBroadcast::HasNot, "value" }, { Resources::CheckBroadcast::Has, "error" } },
            __LINE__ );
    }

    // Add the item back
    auto newItemId = addItem( m_remote, item, __LINE__, nullptr );

    {
        runExpressionsList( answer, true );
        checkNoError( answer, __FILE__, __LINE__ );

        JsonArray expressions;
        checkExpressions( answer, expressions, __LINE__ );
        checkExpressionByName( expressions,
                               expressionName,
                               { { "value", updatedExpressionValue }, { Resources::CheckBroadcast::HasNot, "error" } },
                               __LINE__ );
    }
    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, eventTracking_deviceNameUpdated_T4_2 ) /* 4.2 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const auto expressionName = "deviceNameUpdated";

    const std::string deviceName = test_info_->name();
    auto deviceUPtr = std::make_unique< CoreD::Model::Device >(
        createDevice( m_remote, deviceName, "siren", "siren", "siren", "", false, __LINE__ ) );
    auto deviceId = addDevice( m_remote, *deviceUPtr, __LINE__ );

    using namespace CoreD::Model;
    const std::string itemName = "sound_volume";
    const int value = 42;

    CoreD::Model::Item item = makeIntItem( deviceId, itemName, value );
    auto itemId = addItem( m_remote, item, __LINE__, nullptr );

    auto code = "return params.sound_volume_minus_42 - 40";
    auto deviceItemClause = JSON( "name"
                                  << "sound_volume_minus_42"
                                  << "deviceName" << deviceName << "itemName" << itemName );
    scenesExpressionSet( expressionName, code, JSON_ARRAY_( deviceItemClause ), {}, false, __LINE__ );

    Json answer;
    const auto updatedExpressionValue = ( value - 40 );
    // check output
    {
        runExpressionsList( answer, true );
        checkNoError( answer, __FILE__, __LINE__ );

        JsonArray expressions;
        checkExpressions( answer, expressions, __LINE__ );
        checkExpressionByName( expressions,
                               expressionName,
                               { { "value", updatedExpressionValue }, { Resources::CheckBroadcast::HasNot, "error" } },
                               __LINE__ );
    }

    auto updatedDeviceUPtr = std::make_unique< CoreD::Model::Device >(
        createDevice( m_remote, deviceName + "_with_modified_name", "siren", "siren", "siren", "", false, __LINE__ ) );
    m_remote.modifyDevice( deviceId, *updatedDeviceUPtr );
    checkBroadcast( deviceId, "hub.device.updated", { { Resources::CheckBroadcast::Has, "name" } }, __LINE__ );

    {
        runExpressionsList( answer, true );
        checkNoError( answer, __FILE__, __LINE__ );

        JsonArray expressions;
        checkExpressions( answer, expressions, __LINE__ );
        checkExpressionByName(
            expressions,
            expressionName,
            { { Resources::CheckBroadcast::HasNot, "value" }, { Resources::CheckBroadcast::Has, "error" } },
            __LINE__ );
    }

    // change back to a previous name
    m_remote.modifyDevice( deviceId, *deviceUPtr );
    checkBroadcast( deviceId, "hub.device.updated", { { Resources::CheckBroadcast::Has, "name" } }, __LINE__ );

    {
        runExpressionsList( answer, true );
        checkNoError( answer, __FILE__, __LINE__ );

        JsonArray expressions;
        checkExpressions( answer, expressions, __LINE__ );
        checkExpressionByName( expressions,
                               expressionName,
                               { { "value", updatedExpressionValue }, { Resources::CheckBroadcast::HasNot, "error" } },
                               __LINE__ );
    }
    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, eventTracking_isItemState_itemReachedExpressionValue_T5_1_1 ) /* 5.1.1 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const std::string expressionName = "itemReachedExprValue";

    // set up: device and item with value = 1, constant expression with value = 3;
    // test: update item.value to 3; scene should run;
    auto deviceId = makeDeviceSiren( __LINE__ );

    using namespace CoreD::Model;
    const std::string targetItemName = "item_from_scene_block";
    const int value = 2, expressionValue = 3;

    CoreD::Model::Item item2 = makeIntItem( deviceId, targetItemName, value );
    auto targetItemId = addItem( m_remote, item2, __LINE__, nullptr );

    auto code = "return " + std::to_string( expressionValue );
    scenesExpressionSet( expressionName, code, {}, {}, false, __LINE__ );

    // format and create a scene
    auto isItemStateBlock = replaceTokensInString( isItemStateWhenBlock,
                                                   { { "__ITEM_ID__", targetItemId },
                                                     { "__TYPE__", "expression" },
                                                     { "__VALUE__", "\"" + expressionName + "\"" } } );
    auto request = replaceTokens( createSceneRequest,
                                  { { "__NAME__", "isItemStateWithExpression" },
                                    { "__WHEN_BLOCKS__", isItemStateBlock },
                                    { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId = createScene( request, __LINE__ );

    // emit item.updated event
    setItemValue( m_remote, targetItemId, 3, __LINE__ );

    // the scene should be started by isItemState block
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    deleteScene( sceneId );
    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, eventTracking_isItemState_expressionReachedItemValue_T5_1_2 ) /* 5.1.2 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const std::string expressionName = "exprReachedItemValue";

    // set up: device and item with value = 1, constant expression with value = 3;
    // test: update item.value to 3; scene should run;
    auto deviceId = makeDeviceSiren( __LINE__ );

    using namespace CoreD::Model;
    const std::string itemName = "item_from_expression", targetItemName = "item_from_scene_block";
    const int value = 0, targetItemValue = 5;

    CoreD::Model::Item item1 = makeIntItem( deviceId, itemName, value );
    auto expressionItemId = addItem( m_remote, item1, __LINE__, nullptr );
    CoreD::Model::Item item2 = makeIntItem( deviceId, targetItemName, targetItemValue );
    auto targetItemId = addItem( m_remote, item2, __LINE__, nullptr );

    auto code = "return params.expression_item";
    auto itemClause = JSON( "name"
                            << "expression_item"
                            << "_id" << expressionItemId );
    scenesExpressionSet( expressionName, code, {}, JSON_ARRAY_( itemClause ), false, __LINE__ );

    // format and create a scene
    auto isItemStateBlock = replaceTokensInString( isItemStateWhenBlock,
                                                   { { "__ITEM_ID__", targetItemId },
                                                     { "__TYPE__", "expression" },
                                                     { "__VALUE__", "\"" + expressionName + "\"" } } );
    auto request = replaceTokens( createSceneRequest,
                                  { { "__NAME__", "isItemStateWithExpression" },
                                    { "__WHEN_BLOCKS__", isItemStateBlock },
                                    { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    // emit item.updated event for an expression item; it changes the value of an expression, ItemEvent will be raised;
    setItemValue( m_remote, expressionItemId, 5, __LINE__ );

    // the scene should be started by isItemState block
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    deleteScene( sceneId );
    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, eventTracking_compareNumbers_itemReachesExpression_T5_2_1 ) /* 5.2.1 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const std::string expressionName = "cnItemReachExpr";

    auto deviceId = makeDeviceSiren( __LINE__ );

    using namespace CoreD::Model;
    const std::string targetItemName = "item_from_scene_block";
    const int targetValue = 2, expressionValue = 3;

    CoreD::Model::Item item2 = makeIntItem( deviceId, targetItemName, targetValue );
    auto targetItemId = addItem( m_remote, item2, __LINE__, nullptr );

    auto code = "return " + std::to_string( expressionValue );
    scenesExpressionSet( expressionName, code, {}, {}, false, __LINE__ );

    // format and create a scene
    auto block = replaceTokensInString( compareNumbersWhenBlock,
                                        { { "__SOURCE_NAME__", "item" },
                                          { "__SOURCE_TYPE__", "item" },
                                          { "__SOURCE_VALUE__", quote( targetItemId ) },
                                          { "__COMPARATOR__", ">" },
                                          { "__TYPE__", "expression" },
                                          { "__VALUE__", "\"" + expressionName + "\"" } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "compareNumbersWithExpression" }, { "__WHEN_BLOCKS__", block }, { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    // emit item.updated event
    setItemValue( m_remote, targetItemId, expressionValue + 1, __LINE__ );

    // the scene should be started by compareNumbers block
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    deleteScene( sceneId );
    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, eventTracking_compareNumbers_expressionReachedItemValue_T5_2_2 ) /* 5.2.2 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const std::string expressionName = "cnExprReachItem";

    auto deviceId = makeDeviceSiren( __LINE__ );

    using namespace CoreD::Model;
    const std::string itemName = "item_from_expression", targetItemName = "item_from_scene_block";
    // raise the scene if ( targetItemValue > expressionValue )
    const int expressionValue = 5, targetItemValue = 5;

    CoreD::Model::Item item1 = makeIntItem( deviceId, itemName, expressionValue );
    auto expressionItemId = addItem( m_remote, item1, __LINE__, nullptr );
    CoreD::Model::Item item2 = makeIntItem( deviceId, targetItemName, targetItemValue );
    auto targetItemId = addItem( m_remote, item2, __LINE__, nullptr );

    auto code = "return params.expression_item";
    auto itemClause = JSON( "name"
                            << "expression_item"
                            << "_id" << expressionItemId );
    scenesExpressionSet( expressionName, code, {}, JSON_ARRAY_( itemClause ), false, __LINE__ );

    // format and create a scene
    auto block = replaceTokensInString( compareNumbersWhenBlock,
                                        { { "__SOURCE_NAME__", "item" },
                                          { "__SOURCE_TYPE__", "item" },
                                          { "__SOURCE_VALUE__", quote( targetItemId ) },
                                          { "__COMPARATOR__", ">" },
                                          { "__TYPE__", "expression" },
                                          { "__VALUE__", quote( expressionName ) } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "compareNumbersWithExpression" }, { "__WHEN_BLOCKS__", block }, { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    // emit item.updated event for an expression item; it changes the value of an expression, compareNumbers will be
    // raised;
    setItemValue( m_remote, expressionItemId, targetItemValue - 1, __LINE__ );

    // the scene should be started by compareNumbers block
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    deleteScene( sceneId );
    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, eventTracking_compareNumbers_LeftExpressionVsRightItem_T5_2_3 ) /* 5.2.3 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const std::string expressionName = "cnItemReachExpr";

    auto deviceId = makeDeviceSiren( __LINE__ );

    using namespace CoreD::Model;
    const std::string targetItemName = "item_from_scene_block";
    int expressionValue = 2;
    int itemValue = 3;

    const bool expressionExists = true;
    const bool expectBroadcast = true;

    auto setExpression =
        [this]( const std::string& name, int value, bool expressionExists, bool expectedBroadcast, int codeLine ) {
            auto code = "return " + std::to_string( value );
            scenesExpressionSetExt(
                name, code, {}, {}, expressionExists, "int", {}, false, expectedBroadcast, codeLine );
        };

    CoreD::Model::Item item = makeIntItem( deviceId, targetItemName, itemValue );
    auto targetItemId = addItem( m_remote, item, __LINE__, nullptr );

    setExpression( expressionName, expressionValue, !expressionExists, expectBroadcast, __LINE__ );

    // format and create a scene
    auto block = replaceTokensInString( compareNumbersWhenBlock,
                                        { { "__SOURCE_NAME__", "expression" },
                                          { "__SOURCE_TYPE__", "expression" },
                                          { "__SOURCE_VALUE__", "\"" + expressionName + "\"" },
                                          { "__COMPARATOR__", "==" },
                                          { "__TYPE__", "item" },
                                          { "__VALUE__", quote( targetItemId ) } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "compareNumbersWithExpression" }, { "__WHEN_BLOCKS__", block }, { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    // Left expression reaches right item
    expressionValue = itemValue;
    setExpression( expressionName, expressionValue, expressionExists, !expectBroadcast, __LINE__ );

    checkBroadcasts(
        { [this, &sceneId]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, sceneId, "started" ); },
          [this, &sceneId]( const Json& broadcast ) {
              return isRunProgressBroadcast( broadcast, sceneId, "finished" );
          },
          [&]( const Json& broadcast ) { return isExpressionChangedBroadcast( broadcast, expressionName ); } },
        __LINE__ );

    // Reset event
    expressionValue = itemValue + 1;
    setExpression( expressionName, expressionValue, expressionExists, !expectBroadcast, __LINE__ );
    checkBroadcasts(
        { [&]( const Json& broadcast ) { return isExpressionChangedBroadcast( broadcast, expressionName ); } },
        __LINE__ );

    // Right item reaches left expression
    itemValue = expressionValue;
    setItemValue( m_remote, targetItemId, itemValue, __LINE__ );

    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    deleteScene( sceneId );
    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, eventTracking_compareNumbers_LeftExpressionReachesRightExpression_T5_2_4 ) /* 5.2.4 */ {
    using namespace std::placeholders;

    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const std::string leftExpressionName = "leftExpression";
    const std::string rightExpressionName = "rightExpression";

    const bool expressionExists = true;
    const bool expectBroadcast = true;

    auto setExpression =
        [this]( const std::string& name, int value, bool expressionExists, bool expectedBroadcast, int codeLine ) {
            auto code = "return " + std::to_string( value );
            scenesExpressionSetExt(
                name, code, {}, {}, expressionExists, "int", {}, false, expectedBroadcast, codeLine );
        };

    const int leftValue = 2;
    const int rightValue = 3;

    setExpression( leftExpressionName, leftValue, !expressionExists, expectBroadcast, __LINE__ );
    setExpression( rightExpressionName, rightValue, !expressionExists, expectBroadcast, __LINE__ );

    // format and create a scene
    auto block = replaceTokensInString( compareNumbersWhenBlock,
                                        { { "__SOURCE_NAME__", "expression" },
                                          { "__SOURCE_TYPE__", "expression" },
                                          { "__SOURCE_VALUE__", quote( leftExpressionName ) },
                                          { "__COMPARATOR__", ">" },
                                          { "__TYPE__", "expression" },
                                          { "__VALUE__", quote( rightExpressionName ) } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "compareNumbersWithExpression" }, { "__WHEN_BLOCKS__", block }, { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    // update expression
    setExpression( leftExpressionName, rightValue + 1, expressionExists, !expectBroadcast, __LINE__ );

    checkBroadcasts(
        { [this, &sceneId]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, sceneId, "started" ); },
          [this, &sceneId]( const Json& broadcast ) {
              return isRunProgressBroadcast( broadcast, sceneId, "finished" );
          },
          [&]( const Json& broadcast ) { return isExpressionChangedBroadcast( broadcast, leftExpressionName ); } },
        __LINE__ );

    deleteScene( sceneId );
    deleteSceneExpression( leftExpressionName, __LINE__ );
    deleteSceneExpression( rightExpressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, eventTracking_compareNumbers_RightExpressionReachesLeftExpression_T5_2_5 ) /* 5.2.5 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const std::string leftExpressionName = "leftExpressionName";
    const std::string rightExpressionName = "rightExpressionName";

    auto deviceId = makeDeviceSiren( __LINE__ );

    using namespace CoreD::Model;
    const std::string leftItemName = "item_from_left_expression";
    const std::string rightItemName = "item_from_right_expression";

    // raise the scene if ( leftValue > rightValue )
    const int leftValue = 5;
    const int rightValue = 5;

    auto setupItem =
        [&]( const std::string& expressionName, int value, int line ) {
            const std::string itemName = "item_from_" + expressionName;
            CoreD::Model::Item item = makeIntItem( deviceId, itemName, value );
            auto itemId = addItem( m_remote, item, line, nullptr );

            auto code = "return params.expression_item";
            auto itemClause = JSON( "name"
                                    << "expression_item"
                                    << "_id" << itemId );
            scenesExpressionSet( expressionName, code, {}, JSON_ARRAY_( itemClause ), false, line );
            return itemId;
        };

    auto leftItemId = setupItem( leftExpressionName, leftValue, __LINE__ );
    auto rightItemId = setupItem( rightExpressionName, rightValue, __LINE__ );

    // format and create a scene
    auto block = replaceTokensInString( compareNumbersWhenBlock,
                                        { { "__SOURCE_NAME__", "expression" },
                                          { "__SOURCE_TYPE__", "expression" },
                                          { "__SOURCE_VALUE__", quote( leftExpressionName ) },
                                          { "__COMPARATOR__", ">" },
                                          { "__TYPE__", "expression" },
                                          { "__VALUE__", quote( rightExpressionName ) } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "compareNumbersWithExpression" }, { "__WHEN_BLOCKS__", block }, { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    // change item value to emit item.updated event and trigger event
    setItemValue( m_remote, rightItemId, rightValue - 1, __LINE__ );

    // the scene should be started by compareNumbers block
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    deleteScene( sceneId );
    deleteSceneExpression( leftExpressionName, __LINE__ );
    deleteSceneExpression( rightExpressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, expressionDoesNotExist_T5_3 ) /* 5.3 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const std::string expressionName = "exprDoesNotExist";

    auto deviceId = makeDeviceSiren( __LINE__ );

    using namespace CoreD::Model;
    const std::string targetItemName = "item_from_scene_block";
    const int targetItemValue = 5;

    CoreD::Model::Item item2 = makeIntItem( deviceId, targetItemName, targetItemValue );
    auto targetItemId = addItem( m_remote, item2, __LINE__, nullptr );

    // format and create a scene
    auto block = replaceTokensInString( compareNumbersWhenBlock,
                                        { { "__SOURCE_NAME__", "item" },
                                          { "__SOURCE_TYPE__", "item" },
                                          { "__SOURCE_VALUE__", quote( targetItemId ) },
                                          { "__COMPARATOR__", ">" },
                                          { "__TYPE__", "expression" },
                                          { "__VALUE__", "\"" + expressionName + "\"" } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "expressionThatDoesNotExist" }, { "__WHEN_BLOCKS__", block }, { "__THEN_BLOCKS__", "" } } );

    ASSERT_NO_THROW( sendRequestAndCheckFailedAnswer( request, __LINE__ ) )
        << "Should deny to create the scene with non-existent expression name used";
}

TEST_F( ScenesExpressionsTests, eventTracking_isItemState_itemScalableReachedExpressionValue_T5_4_1 ) /* 5.4.1 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const std::string expressionName = "itemScalableReachedExprValue";

    // set up: device and item with value = 1, constant expression with value = 3;
    // test: update item.value to 3; scene should run;
    auto deviceId = makeDeviceThermostat( __LINE__ );

    using namespace CoreD::Model;
    const char* defaultScale = ValueScales::Fahrenheit;
    const std::string targetItemName = "item_from_scene_block";
    const int targetValue = 2, expressionValue = 3;

    Item item2 = makeTemperatureItem( deviceId, targetItemName, targetValue );
    auto targetItemId = addItem( m_remote, item2, __LINE__, nullptr );

    auto code =
        "return { value = " + std::to_string( expressionValue ) + ", scale = \"" + std::string{ defaultScale } + "\" }";
    scenesExpressionSet( expressionName, code, {}, {}, false, __LINE__ );

    // format and create a scene
    auto isItemStateBlock = replaceTokensInString( isItemStateWhenBlock,
                                                   { { "__ITEM_ID__", targetItemId },
                                                     { "__TYPE__", "expression" },
                                                     { "__VALUE__", "\"" + expressionName + "\"" } } );
    auto request = replaceTokens( createSceneRequest,
                                  { { "__NAME__", "isItemStateWithExpression" },
                                    { "__WHEN_BLOCKS__", isItemStateBlock },
                                    { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    // emit item.updated event, item and expression values should match
    setItemValue( m_remote, targetItemId, expressionValue, __LINE__, defaultScale );

    // the scene should be started by isItemState block
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    deleteScene( sceneId );
    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, eventTracking_isItemState_expressionReachedItemScalableValue_T5_4_2 ) /* 5.4.2 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const std::string expressionName = "exprReachedItemScalableValue";

    // set up: device and item with value = 1, constant expression with value = 3;
    // test: update item.value to 3; scene should run;
    auto deviceId = makeDeviceThermostat( __LINE__ );

    using namespace CoreD::Model;
    const std::string itemName = "item_from_expression", targetItemName = "item_from_scene_block";
    const int expressionValue = 0, targetItemValue = 5;

    const char* defaultScale = ValueScales::Fahrenheit;

    Item item1 = makeTemperatureItem( deviceId, itemName, expressionValue, defaultScale );
    auto expressionItemId = addItem( m_remote, item1, __LINE__, nullptr );
    Item item2 = makeTemperatureItem( deviceId, targetItemName, targetItemValue, defaultScale );
    auto targetItemId = addItem( m_remote, item2, __LINE__, nullptr );

    auto code = "return params.expression_item";
    auto itemClause = JSON( "name"
                            << "expression_item"
                            << "_id" << expressionItemId );
    scenesExpressionSet( expressionName, code, {}, JSON_ARRAY_( itemClause ), false, __LINE__ );

    // format and create a scene
    auto isItemStateBlock = replaceTokensInString( isItemStateWhenBlock,
                                                   { { "__ITEM_ID__", targetItemId },
                                                     { "__TYPE__", "expression" },
                                                     { "__VALUE__", "\"" + expressionName + "\"" } } );
    auto request = replaceTokens( createSceneRequest,
                                  { { "__NAME__", "isItemStateWithExpression" },
                                    { "__WHEN_BLOCKS__", isItemStateBlock },
                                    { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    // emit item.updated event for an expression item; it changes the value of an expression, ItemEvent will be raised;
    int newExpressionValue = targetItemValue;
    setItemValue( m_remote, expressionItemId, newExpressionValue, __LINE__, defaultScale );

    // the scene should be started by isItemState block
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    deleteScene( sceneId );
    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, eventTracking_compareNumbers_scalableItemReachesExpression_T5_5_1 ) /* 5.5.1 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const std::string expressionName = "cnScalableItemReachExpr";

    // make device
    auto deviceId = makeDeviceThermostat( __LINE__ );

    using namespace CoreD::Model;
    const char* defaultScale = ValueScales::Fahrenheit;
    const std::string targetItemName = "item_from_scene_block";
    const int targetValue = 2, expressionValue = 3;

    Item item2 = makeTemperatureItem( deviceId, targetItemName, targetValue, defaultScale );
    auto targetItemId = addItem( m_remote, item2, __LINE__, nullptr );

    auto code = "return { value = " + std::to_string( expressionValue ) + ", scale = \"" + defaultScale + "\" }";
    scenesExpressionSet( expressionName, code, {}, {}, false, __LINE__ );

    // format and create a scene
    auto block = replaceTokensInString( compareNumbersWhenBlock,
                                        { { "__SOURCE_NAME__", "item" },
                                          { "__SOURCE_TYPE__", "item" },
                                          { "__SOURCE_VALUE__", quote( targetItemId ) },
                                          { "__COMPARATOR__", ">" },
                                          { "__TYPE__", "expression" },
                                          { "__VALUE__", "\"" + expressionName + "\"" } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "compareNumbersWithExpression" }, { "__WHEN_BLOCKS__", block }, { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    // emit item.updated event
    setItemValue( m_remote, targetItemId, expressionValue + 1, __LINE__, defaultScale );

    // the scene should be started by compareNumbers block
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    deleteScene( sceneId );
    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, eventTracking_compareNumbers_expressionReachedItemValue_T5_5_2 ) /* 5.5.2 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const std::string expressionName = "cnExprReachScalableItem";

    // make device
    auto deviceId = makeDeviceThermostat( __LINE__ );

    using namespace CoreD::Model;
    const std::string itemName = "item_from_expression", targetItemName = "item_from_scene_block";
    // raise the scene if ( targetItemValue > expressionValue )
    const int expressionValue = 5, targetItemValue = 5;

    const char* defaultScale = ValueScales::Fahrenheit;
    // make items
    Item item1 = makeTemperatureItem( deviceId, itemName, expressionValue, defaultScale );
    auto expressionItemId = addItem( m_remote, item1, __LINE__, nullptr );
    Item item2 = makeTemperatureItem( deviceId, targetItemName, targetItemValue, defaultScale );
    auto targetItemId = addItem( m_remote, item2, __LINE__, nullptr );

    auto code = "return params.expression_item";
    auto itemClause = JSON( "name"
                            << "expression_item"
                            << "_id" << expressionItemId );
    scenesExpressionSet( expressionName, code, {}, JSON_ARRAY_( itemClause ), false, __LINE__ );

    // format and create a scene
    auto block = replaceTokensInString( compareNumbersWhenBlock,
                                        { { "__SOURCE_NAME__", "item" },
                                          { "__SOURCE_TYPE__", "item" },
                                          { "__SOURCE_VALUE__", quote( targetItemId ) },
                                          { "__COMPARATOR__", ">" },
                                          { "__TYPE__", "expression" },
                                          { "__VALUE__", "\"" + expressionName + "\"" } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "compareNumbersWithExpression" }, { "__WHEN_BLOCKS__", block }, { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    setItemValue( m_remote, expressionItemId, expressionValue - 5, __LINE__, defaultScale );

    // the scene should be started by compareNumbers block
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    deleteScene( sceneId );
    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, eventTracking_compareStringsForItemVsExpression ) /* 5.6.1 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const std::string expressionName = "stringItemReachExpr";

    // make device
    auto device = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::String, __LINE__ );
    std::string itemId = device.m_itemId;
    std::string expressionValue = "low";

    auto code = "return \"" + expressionValue + "\"";
    scenesExpressionSet( expressionName, code, {}, {}, false, __LINE__ );

    // format and create a scene
    auto block = replaceTokensInString( compareStringsWhenBlock,
                                        { { "__SOURCE_NAME__", "item" },
                                          { "__SOURCE_TYPE__", "item" },
                                          { "__SOURCE_VALUE__", quote( itemId ) },
                                          { "__COMPARATOR__", "==" },
                                          { "__TYPE__", "expression" },
                                          { "__VALUE__", expressionName } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "compareStringsWithExpression" }, { "__WHEN_BLOCKS__", block }, { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    // emit item.updated event
    setItemValue( m_remote, itemId, expressionValue, __LINE__ );

    // the scene should be started by compareNumbers block
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    deleteScene( sceneId );
    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, eventTracking_compareStringsForExpressionVsExpression ) /* 5.6.2 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );

    const std::string leftExpressionName = "leftExpression";
    const std::string rightExpressionName = "rightExpression";

    const bool expressionExists = true;
    const bool expectBroadcast = true;

    auto setExpression = [this]( const std::string& name,
                                 const std::string& value,
                                 bool expressionExists,
                                 bool expectedBroadcast,
                                 int codeLine ) {
        auto code = "return \"" + value + "\"";
        scenesExpressionSetExt(
            name, code, {}, {}, expressionExists, "string", {}, false, expectedBroadcast, codeLine );
    };

    const std::string leftValue = "low";
    const std::string rightValue = "high";

    setExpression( leftExpressionName, leftValue, !expressionExists, expectBroadcast, __LINE__ );
    setExpression( rightExpressionName, rightValue, !expressionExists, expectBroadcast, __LINE__ );

    // format and create a scene
    auto block = replaceTokensInString( compareStringsWhenBlock,
                                        { { "__SOURCE_NAME__", "expression" },
                                          { "__SOURCE_TYPE__", "expression" },
                                          { "__SOURCE_VALUE__", quote( leftExpressionName ) },
                                          { "__COMPARATOR__", "==" },
                                          { "__TYPE__", "expression" },
                                          { "__VALUE__", rightExpressionName } } );
    auto request = replaceTokens( createSceneRequest,
                                  { { "__NAME__", "compareStringsForExpressionVsExpression" },
                                    { "__WHEN_BLOCKS__", block },
                                    { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    // update expression
    setExpression( leftExpressionName, rightValue, expressionExists, !expectBroadcast, __LINE__ );

    checkBroadcasts(
        { [this, &sceneId]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, sceneId, "started" ); },
          [this, &sceneId]( const Json& broadcast ) {
              return isRunProgressBroadcast( broadcast, sceneId, "finished" );
          },
          [&]( const Json& broadcast ) { return isExpressionChangedBroadcast( broadcast, leftExpressionName ); } },
        __LINE__ );

    deleteScene( sceneId );
    deleteSceneExpression( leftExpressionName, __LINE__ );
    deleteSceneExpression( rightExpressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, eventTracking_compareStringsForExpressionVsItem_T5_6_3 ) /* 5.6.3 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );

    using namespace CoreD::Model;
    const std::string expressionName = "Name_5.6.3";

    auto device = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::String, __LINE__ );
    std::string itemId = device.m_itemId;

    std::string expressionValue = "value1";
    std::string itemValue = "";

    const bool expressionExists = true;
    const bool expectBroadcast = true;

    auto setExpression = [this]( const std::string& name,
                                 const std::string& value,
                                 bool expressionExists,
                                 bool expectedBroadcast,
                                 int codeLine ) {
        auto code = "return \"" + value + "\"";
        scenesExpressionSetExt(
            name, code, {}, {}, expressionExists, "string", {}, false, expectedBroadcast, codeLine );
    };

    setExpression( expressionName, expressionValue, !expressionExists, expectBroadcast, __LINE__ );

    // format and create a scene
    auto block = replaceTokensInString( compareStringsWhenBlock,
                                        { { "__SOURCE_NAME__", "expression" },
                                          { "__SOURCE_TYPE__", "expression" },
                                          { "__SOURCE_VALUE__", "\"" + expressionName + "\"" },
                                          { "__COMPARATOR__", "==" },
                                          { "__TYPE__", "item" },
                                          { "__VALUE__", itemId } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "compareStringsWithExpression" }, { "__WHEN_BLOCKS__", block }, { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    // Right item reaches left expression
    itemValue = expressionValue;
    setItemValue( m_remote, itemId, itemValue, __LINE__ );

    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    // Reset event
    expressionValue = "value2";
    setExpression( expressionName, expressionValue, expressionExists, !expectBroadcast, __LINE__ );
    checkBroadcasts(
        { [&]( const Json& broadcast ) { return isExpressionChangedBroadcast( broadcast, expressionName ); } },
        __LINE__ );

    // Left expression reaches right item
    expressionValue = itemValue;
    setExpression( expressionName, expressionValue, expressionExists, !expectBroadcast, __LINE__ );

    checkBroadcasts(
        { [this, &sceneId]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, sceneId, "started" ); },
          [this, &sceneId]( const Json& broadcast ) {
              return isRunProgressBroadcast( broadcast, sceneId, "finished" );
          },
          [&]( const Json& broadcast ) { return isExpressionChangedBroadcast( broadcast, expressionName ); } },
        __LINE__ );

    deleteScene( sceneId );
    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, eventTracking_stringOperationForItemVsExpression ) /* 5.7.1 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const std::string expressionName = "stringOperationReachExpr";

    // make device
    const auto device = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::String, __LINE__ );
    const std::string itemId = device.m_itemId;
    const std::string expressionValue = "some_test_string";

    const auto expressionCode = "return \"" + expressionValue + "\"";
    scenesExpressionSet( expressionName, expressionCode, {}, {}, false, __LINE__ );

    // format and create a scene
    const auto whenBlock = replaceTokensInString( itemOperationWhenBlock,
                                                  { { "__METHOD__", "stringOperation" },
                                                    { "__SOURCE_NAME__", "item" },
                                                    { "__SOURCE_TYPE__", "item" },
                                                    { "__SOURCE_VALUE__", quote( itemId ) },
                                                    { "__OPERATION__", "begin" },
                                                    { "__TYPE__", "expression" },
                                                    { "__VALUE__", '\"' + expressionName + '\"' } } );
    const auto request = replaceTokens( createSceneRequest,
                                        { { "__NAME__", "stringOperationWithExpression" },
                                          { "__WHEN_BLOCKS__", whenBlock },
                                          { "__THEN_BLOCKS__", "" } } );

    const SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    // emit item.updated event
    setItemValue( m_remote, itemId, expressionValue, __LINE__ );

    // the scene should be started by compareNumbers block
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    deleteScene( sceneId );
    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, eventTracking_stringOperationForExpressionVsExpression ) /* 5.7.2 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );

    const bool expressionExists = true;
    const bool expectBroadcast = true;

    auto setExpression = [this]( const std::string& name,
                                 const std::string value,
                                 bool expressionExists,
                                 bool expectedBroadcast,
                                 int codeLine ) {
        auto code = "return \"" + value + "\"";
        scenesExpressionSetExt(
            name, code, {}, {}, expressionExists, "string", {}, false, expectedBroadcast, codeLine );
    };

    const std::string expressionValue = "some_test_string";

    const std::string expressionSourceName = "expressionSource";
    setExpression( expressionSourceName, "", !expressionExists, expectBroadcast, __LINE__ );

    const std::string expressionTargetName = "stringOperationReachExpr";

    setExpression( expressionTargetName, expressionValue, !expressionExists, expectBroadcast, __LINE__ );
    const auto expressionCode = "return \"" + expressionValue + "\"";

    // format and create a scene
    const auto whenBlock = replaceTokensInString( itemOperationWhenBlock,
                                                  { { "__METHOD__", "stringOperation" },
                                                    { "__SOURCE_NAME__", "expression" },
                                                    { "__SOURCE_TYPE__", "expression" },
                                                    { "__SOURCE_VALUE__", quote( expressionSourceName ) },
                                                    { "__OPERATION__", "begin" },
                                                    { "__TYPE__", "expression" },
                                                    { "__VALUE__", quote( expressionTargetName ) } } );
    const auto request = replaceTokens( createSceneRequest,
                                        { { "__NAME__", "stringOperationWithExpression" },
                                          { "__WHEN_BLOCKS__", whenBlock },
                                          { "__THEN_BLOCKS__", "" } } );

    const SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    setExpression( expressionSourceName, expressionValue, expressionExists, !expectBroadcast, __LINE__ );

    checkBroadcasts(
        { [this, &sceneId]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, sceneId, "started" ); },
          [this, &sceneId]( const Json& broadcast ) {
              return isRunProgressBroadcast( broadcast, sceneId, "finished" );
          },
          [&]( const Json& broadcast ) { return isExpressionChangedBroadcast( broadcast, expressionSourceName ); } },
        __LINE__ );

    deleteScene( sceneId );
    deleteSceneExpression( expressionTargetName, __LINE__ );
    deleteSceneExpression( expressionSourceName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, eventTracking_stringOperationForExpressionVsItem ) /* 5.7.3 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const std::string expressionName = "stringOperationReachExpr";

    // make device
    const auto device = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::String, __LINE__ );
    const std::string itemId = device.m_itemId;
    std::string expressionValue = "some_test_string";

    const bool expressionExists = true;
    const bool expectBroadcast = true;

    auto setExpression = [this]( const std::string& name,
                                 const std::string& value,
                                 bool expressionExists,
                                 bool expectedBroadcast,
                                 int codeLine ) {
        auto code = "return \"" + value + "\"";
        scenesExpressionSetExt(
            name, code, {}, {}, expressionExists, "string", {}, false, expectedBroadcast, codeLine );
    };

    setExpression( expressionName, expressionValue, !expressionExists, expectBroadcast, __LINE__ );

    // format and create a scene
    const auto whenBlock = replaceTokensInString( itemOperationWhenBlock,
                                                  { { "__METHOD__", "stringOperation" },
                                                    { "__SOURCE_NAME__", "item" },
                                                    { "__SOURCE_TYPE__", "item" },
                                                    { "__SOURCE_VALUE__", quote( itemId ) },
                                                    { "__OPERATION__", "begin" },
                                                    { "__TYPE__", "expression" },
                                                    { "__VALUE__", '\"' + expressionName + '\"' } } );
    const auto request = replaceTokens( createSceneRequest,
                                        { { "__NAME__", "stringOperationWithExpression" },
                                          { "__WHEN_BLOCKS__", whenBlock },
                                          { "__THEN_BLOCKS__", "" } } );

    const SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    // Item reaches expression
    setItemValue( m_remote, itemId, expressionValue, __LINE__ );

    // the scene should be started by compareNumbers block
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    // Reset event
    expressionValue = "another_test_string";
    setItemValue( m_remote, itemId, expressionValue, __LINE__ );

    // Left expression reaches right item
    setExpression( expressionName, expressionValue, expressionExists, !expectBroadcast, __LINE__ );

    checkBroadcasts(
        { [this, &sceneId]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, sceneId, "started" ); },
          [this, &sceneId]( const Json& broadcast ) {
              return isRunProgressBroadcast( broadcast, sceneId, "finished" );
          },
          [&]( const Json& broadcast ) { return isExpressionChangedBroadcast( broadcast, expressionName ); } },
        __LINE__ );

    deleteScene( sceneId );
    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, eventTracking_compareValuesForItemSource ) /* 5.8.1 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const std::string expressionName = "comValuesReachExpr";

    // make device
    const auto device = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::String, __LINE__ );
    const std::string itemId = device.m_itemId;
    const std::string expressionValue = "some_test_string";

    const auto expressionCode = "return \"" + expressionValue + "\"";
    scenesExpressionSet( expressionName, expressionCode, {}, {}, false, __LINE__ );

    // format and create a scene
    const auto whenBlock = replaceTokensInString( compareValuesWhenBlock,
                                                  { { "__SOURCE_NAME__", "item" },
                                                    { "__SOURCE_TYPE__", "item" },
                                                    { "__SOURCE_VALUE__", quote( itemId ) },
                                                    { "__COMPARATOR__", "==" },
                                                    { "__VALUE_TYPE_VALUE__", "string" },
                                                    { "__TYPE__", "expression" },
                                                    { "__UNQUOTED_VALUE__", '\"' + expressionName + '\"' } } );
    const auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "cmpValuesWithExpr" }, { "__WHEN_BLOCKS__", whenBlock }, { "__THEN_BLOCKS__", "" } } );

    LOGGER_TRACE( cc::j( request ) );

    const SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    // emit item.updated event
    setItemValue( m_remote, itemId, expressionValue, __LINE__ );

    // the scene should be started by compareNumbers block
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    deleteScene( sceneId );
    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, eventTracking_compareValuesForExpressionSource ) /* 5.8.2 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );

    const bool expressionExists = true;
    const bool expectBroadcast = true;

    auto setExpression = [this]( const std::string& name,
                                 const std::string value,
                                 bool expressionExists,
                                 bool expectedBroadcast,
                                 int codeLine ) {
        auto code = "return \"" + value + "\"";
        scenesExpressionSetExt(
            name, code, {}, {}, expressionExists, "string", {}, false, expectedBroadcast, codeLine );
    };

    const std::string expressionValue = "some_test_string";

    const std::string expressionSourceName = "expressionSource";
    setExpression( expressionSourceName, "", !expressionExists, expectBroadcast, __LINE__ );

    const std::string expressionTargetName = "comValuesReachExpr";

    setExpression( expressionTargetName, expressionValue, !expressionExists, expectBroadcast, __LINE__ );

    // format and create a scene
    const auto whenBlock = replaceTokensInString( compareValuesWhenBlock,
                                                  { { "__SOURCE_NAME__", "expression" },
                                                    { "__SOURCE_TYPE__", "expression" },
                                                    { "__SOURCE_VALUE__", quote( expressionSourceName ) },
                                                    { "__COMPARATOR__", "==" },
                                                    { "__VALUE_TYPE_VALUE__", "string" },
                                                    { "__TYPE__", "expression" },
                                                    { "__UNQUOTED_VALUE__", '\"' + expressionTargetName + '\"' } } );
    const auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "cmpValuesWithExpr" }, { "__WHEN_BLOCKS__", whenBlock }, { "__THEN_BLOCKS__", "" } } );

    LOGGER_TRACE( cc::j( request ) );

    const SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    setExpression( expressionSourceName, expressionValue, expressionExists, !expectBroadcast, __LINE__ );

    checkBroadcasts(
        { [this, &sceneId]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, sceneId, "started" ); },
          [this, &sceneId]( const Json& broadcast ) {
              return isRunProgressBroadcast( broadcast, sceneId, "finished" );
          },
          [&]( const Json& broadcast ) { return isExpressionChangedBroadcast( broadcast, expressionSourceName ); } },
        __LINE__ );

    deleteScene( sceneId );
    deleteSceneExpression( expressionTargetName, __LINE__ );
    deleteSceneExpression( expressionSourceName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, eventTracking_compareValuesForExpressionVsItem ) /* 5.8.3 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );

    using namespace CoreD::Model;
    const std::string expressionName = "Name_5.8.3";

    auto device = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::String, __LINE__ );
    std::string itemId = device.m_itemId;

    std::string expressionValue = "value1";
    std::string itemValue = "";

    const bool expressionExists = true;
    const bool expectBroadcast = true;

    auto setExpression = [this]( const std::string& name,
                                 const std::string& value,
                                 bool expressionExists,
                                 bool expectedBroadcast,
                                 int codeLine ) {
        auto code = "return \"" + value + "\"";
        scenesExpressionSetExt(
            name, code, {}, {}, expressionExists, "string", {}, false, expectedBroadcast, codeLine );
    };

    setExpression( expressionName, expressionValue, !expressionExists, expectBroadcast, __LINE__ );

    const auto whenBlock = replaceTokensInString( compareValuesWhenBlock,
                                                  { { "__SOURCE_NAME__", "expression" },
                                                    { "__SOURCE_TYPE__", "expression" },
                                                    { "__SOURCE_VALUE__", quote( expressionName ) },
                                                    { "__COMPARATOR__", "==" },
                                                    { "__VALUE_TYPE_VALUE__", "string" },
                                                    { "__TYPE__", "item" },
                                                    { "__UNQUOTED_VALUE__", quote( itemId ) } } );
    const auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "cmpValuesWithExpr" }, { "__WHEN_BLOCKS__", whenBlock }, { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    // Right item reaches left expression
    itemValue = expressionValue;
    setItemValue( m_remote, itemId, itemValue, __LINE__ );

    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    // Reset event
    expressionValue = "value2";
    setExpression( expressionName, expressionValue, expressionExists, !expectBroadcast, __LINE__ );
    checkBroadcasts(
        { [&]( const Json& broadcast ) { return isExpressionChangedBroadcast( broadcast, expressionName ); } },
        __LINE__ );

    // Left expression reaches right item
    expressionValue = itemValue;
    setExpression( expressionName, expressionValue, expressionExists, !expectBroadcast, __LINE__ );

    checkBroadcasts(
        { [this, &sceneId]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, sceneId, "started" ); },
          [this, &sceneId]( const Json& broadcast ) {
              return isRunProgressBroadcast( broadcast, sceneId, "finished" );
          },
          [&]( const Json& broadcast ) { return isExpressionChangedBroadcast( broadcast, expressionName ); } },
        __LINE__ );

    deleteScene( sceneId );
    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, eventTracking_isItemStateChangedForItemSource ) /* 5.9.1 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    // make device
    const auto device = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );
    const std::string itemId = device.m_itemId;

    const std::string startExpressionName = "isItemStateChangedStartExpr";
    const std::string startExpressionCode = "return 1";
    const std::string finishExpressionName = "isItemStateChangedFinishExpr";
    const std::string finishExpressionCode = "return 2";
    scenesExpressionSet( startExpressionName, startExpressionCode, {}, {}, false, __LINE__ );
    scenesExpressionSet( finishExpressionName, finishExpressionCode, {}, {}, false, __LINE__ );

    // format and create a scene
    const auto whenBlock = replaceTokensInString( isItemStateChangedWhenBlock,
                                                  { { "__SOURCE_NAME__", "item" },
                                                    { "__SOURCE_TYPE__", "item" },
                                                    { "__SOURCE_VALUE__", quote( itemId ) },
                                                    { "__START_TYPE__", "expression" },
                                                    { "__START_VALUE__", quote( startExpressionName ) },
                                                    { "__FINISH_TYPE__", "expression" },
                                                    { "__FINISH_VALUE__", quote( finishExpressionName ) } } );
    const auto request = replaceTokens( createSceneRequest,
                                        { { "__NAME__", "isItemStateChangedWithExpression" },
                                          { "__WHEN_BLOCKS__", whenBlock },
                                          { "__THEN_BLOCKS__", "" } } );

    const SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    // emit item.updated event
    setItemValue( m_remote, itemId, 1, __LINE__ );
    setItemValue( m_remote, itemId, 2, __LINE__ );

    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    deleteScene( sceneId );
    deleteSceneExpression( startExpressionName, __LINE__ );
    deleteSceneExpression( finishExpressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, eventTracking_isItemStateChangedForExpressionSource ) /* 5.9.2 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );

    const bool expressionExists = true;
    const bool expectBroadcast = true;

    auto setExpression =
        [this]( const std::string& name, int value, bool expressionExists, bool expectedBroadcast, int codeLine ) {
            auto code = "return " + std::to_string( value );
            scenesExpressionSetExt(
                name, code, {}, {}, expressionExists, "int", {}, false, expectedBroadcast, codeLine );
        };

    const std::string expressionSourceName = "expressionSource";
    setExpression( expressionSourceName, 0, !expressionExists, expectBroadcast, __LINE__ );

    const std::string startExpressionName = "isItemStateChangedStartExpr";
    const int startExpressionValue = 1;
    const std::string finishExpressionName = "isItemStateChangedFinishExpr";
    const int finishExpressionValue = 2;

    setExpression( startExpressionName, startExpressionValue, !expressionExists, expectBroadcast, __LINE__ );
    setExpression( finishExpressionName, finishExpressionValue, !expressionExists, expectBroadcast, __LINE__ );

    // format and create a scene
    const auto whenBlock = replaceTokensInString( isItemStateChangedWhenBlock,
                                                  { { "__SOURCE_NAME__", "expression" },
                                                    { "__SOURCE_TYPE__", "expression" },
                                                    { "__SOURCE_VALUE__", quote( expressionSourceName ) },
                                                    { "__START_TYPE__", "expression" },
                                                    { "__START_VALUE__", quote( startExpressionName ) },
                                                    { "__FINISH_TYPE__", "expression" },
                                                    { "__FINISH_VALUE__", quote( finishExpressionName ) } } );
    const auto request = replaceTokens( createSceneRequest,
                                        { { "__NAME__", "isItemStateChangedWithExpression" },
                                          { "__WHEN_BLOCKS__", whenBlock },
                                          { "__THEN_BLOCKS__", "" } } );

    const SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    setExpression( expressionSourceName, startExpressionValue, expressionExists, !expectBroadcast, __LINE__ );
    setExpression( expressionSourceName, finishExpressionValue, expressionExists, !expectBroadcast, __LINE__ );

    checkBroadcasts(
        { [this, &sceneId]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, sceneId, "started" ); },
          [this, &sceneId]( const Json& broadcast ) {
              return isRunProgressBroadcast( broadcast, sceneId, "finished" );
          },
          [&]( const Json& broadcast ) { return isExpressionChangedBroadcast( broadcast, expressionSourceName ); },
          [&]( const Json& broadcast ) { return isExpressionChangedBroadcast( broadcast, expressionSourceName ); } },
        __LINE__ );

    deleteScene( sceneId );
    deleteSceneExpression( startExpressionName, __LINE__ );
    deleteSceneExpression( finishExpressionName, __LINE__ );
    deleteSceneExpression( expressionSourceName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, eventTracking_isItemStateChangedIgnoreChangeOfEndExpression ) /* 5.9.3 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );

    const auto device = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );
    const std::string itemId = device.m_itemId;

    const bool expressionExists = true;
    const bool expectBroadcast = true;

    auto setExpression =
        [this]( const std::string& name, int value, bool expressionExists, bool expectedBroadcast, int codeLine ) {
            auto code = "return " + std::to_string( value );
            scenesExpressionSetExt(
                name, code, {}, {}, expressionExists, "int", {}, false, expectedBroadcast, codeLine );
        };

    const std::string finishExpressionName = "isItemStateChangedFinishExpr";
    setExpression( finishExpressionName, 33, !expressionExists, expectBroadcast, __LINE__ );

    // simple scene, any -> value for item
    const auto whenBlock = replaceTokensInString( isItemStateChangedWhenBlock,
                                                  { { "__SOURCE_NAME__", "item" },
                                                    { "__SOURCE_TYPE__", "item" },
                                                    { "__SOURCE_VALUE__", quote( itemId ) },
                                                    { "__START_TYPE__", "string" },
                                                    { "__START_VALUE__", quote( "any" ) },
                                                    { "__FINISH_TYPE__", "expression" },
                                                    { "__FINISH_VALUE__", quote( finishExpressionName ) } } );
    const auto request = replaceTokens( createSceneRequest,
                                        { { "__NAME__", "isItemStateChangedWithExpression" },
                                          { "__WHEN_BLOCKS__", whenBlock },
                                          { "__THEN_BLOCKS__", "" } } );
    const SceneID sceneId = createScene( request, __LINE__ );

    setItemValue( m_remote, itemId, 44, __LINE__ );
    setExpression( finishExpressionName, 44, expressionExists, !expectBroadcast, __LINE__ );
    checkBroadcasts(
        { [&]( const Json& broadcast ) { return isExpressionChangedBroadcast( broadcast, finishExpressionName ); } },
        __LINE__ );

    // make sure scene is not executed
    checkNoBroadcastReceived( "No broadcasts expected", __LINE__, std::chrono::seconds( 2 ) );

    deleteScene( sceneId );
    deleteSceneExpression( finishExpressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, combinationOfDifferentItemEvents_T5_10 ) /* 5.10 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    // make device
    const auto device = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );
    const std::string itemId = device.m_itemId;

    const std::string expressionName = "testExpression";
    const std::string expressionCode = "return 2";
    scenesExpressionSet( expressionName, expressionCode, {}, {}, false, __LINE__ );

    const auto whenBlock1 = replaceTokensInString( isItemStateChangedWhenBlock,
                                                   { { "__SOURCE_NAME__", "item" },
                                                     { "__SOURCE_TYPE__", "item" },
                                                     { "__SOURCE_VALUE__", quote( itemId ) },
                                                     { "__START_TYPE__", "string" },
                                                     { "__START_VALUE__", quote( "any" ) },
                                                     { "__FINISH_TYPE__", "expression" },
                                                     { "__FINISH_VALUE__", quote( expressionName ) } } );
    const auto whenBlock2 = replaceTokensInString( compareNumbersWhenBlock,
                                                   { { "__SOURCE_NAME__", "item" },
                                                     { "__SOURCE_TYPE__", "item" },
                                                     { "__SOURCE_VALUE__", quote( itemId ) },
                                                     { "__COMPARATOR__", "==" },
                                                     { "__TYPE__", "expression" },
                                                     { "__VALUE__", quote( expressionName ) } } );
    const auto whenBlock3 = replaceTokensInString(
        isItemStateWhenBlock,
        { { "__ITEM_ID__", itemId }, { "__TYPE__", "expression" }, { "__VALUE__", quote( expressionName ) } } );
    const auto andOperatorBlock = replaceTokensInString(
        andOperatorWhenBlock, { { "__BLOCKS__", whenBlock1 + ", " + whenBlock2 + ", " + whenBlock3 } } );
    const auto request = replaceTokens( createSceneRequest,
                                        { { "__NAME__", "differentEventsCombination" },
                                          { "__WHEN_BLOCKS__", andOperatorBlock },
                                          { "__THEN_BLOCKS__", "" } } );

    const SceneID sceneId = createScene( request, __LINE__ );

    setItemValue( m_remote, itemId, 2, __LINE__ );

    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    deleteScene( sceneId );
    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, variableExpressionSet_create_T_6_1 ) /* 6.1 */ {
    const auto expressionName = "variableExpressionSet";
    scenesVariableExpressionSetType< int >( expressionName, 123, "hub.expression.added", "int", {}, true, __LINE__ );
    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, variableExpressionListError_T_6_2 ) /* 6.2 */ {
    const auto expressionName = "variableExpressionList";
    const Json value = { { "info", "object instead of string value" } };
    bool isExpectedBroadcast = true;
    Json command = makeVariableExpressionSetCommand( expressionName, value, "string", {}, isExpectedBroadcast );
    auto answer = nma().sendtoHubAndGetAnswer( command );
    checkHasError( answer, __FILE__, __LINE__ );

    Json errorData;
    EXPECT_TRUE( json_get( answer, { "error", "data" }, errorData ) );
    EXPECT_EQ( json_get_str( errorData ), "scenes.expression.type.mismatch" );
}

TEST_F( ScenesExpressionsTests, variableExpressionTypedList_T_6_3 ) /* 6.3 */ {
    const auto expressionName = "variableExpressionList";
    auto value = true;
    bool isExpectedBroadcast = true;
    scenesVariableExpressionSetType< bool >(
        expressionName, value, "hub.expression.added", "bool", {}, isExpectedBroadcast, __LINE__ );
    JsonArray expressions;
    Json answer;
    runExpressionsList( answer, true );
    checkNoError( answer, __FILE__, __LINE__ );
    checkExpressions( answer, expressions, __LINE__ );
    checkExpressionByName(
        expressions, expressionName, { { Resources::CheckBroadcast::HasNot, "error" }, { "value", true } }, __LINE__ );
    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, compareNumbersWithExpression_T6_4 ) /* 6.4 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const std::string expressionName = "expressionValue_T6_4";
    auto deviceId = makeDeviceThermostat( __LINE__ );
    using namespace CoreD::Model;
    const std::string itemName = "item_from_expression", targetItemName = "item_from_scene_block";
    const int expressionValue = 5, targetItemValue = 5, expressionUpdatedValue = 1;
    const char* defaultScale = ValueScales::Fahrenheit;
    Item item = makeTemperatureItem( deviceId, targetItemName, targetItemValue, defaultScale );
    auto targetItemId = addItem( m_remote, item, __LINE__, nullptr );
    bool isExpectedBroadcast = true;
    scenesVariableExpressionSetType< int >(
        expressionName, expressionValue, "hub.expression.added", "int", {}, isExpectedBroadcast, __LINE__ );
    auto block = replaceTokensInString( compareNumbersWhenBlock,
                                        { { "__SOURCE_NAME__", "item" },
                                          { "__SOURCE_TYPE__", "item" },
                                          { "__SOURCE_VALUE__", quote( targetItemId ) },
                                          { "__COMPARATOR__", ">" },
                                          { "__TYPE__", "expression" },
                                          { "__VALUE__", "\"" + expressionName + "\"" } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "compareNumbersWithExpression" }, { "__WHEN_BLOCKS__", block }, { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );
    isExpectedBroadcast = false;
    scenesVariableExpressionSetType< int >(
        expressionName, expressionUpdatedValue, "", "int", {}, isExpectedBroadcast, __LINE__ );

    // the scene should be started by compareNumbers block
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
    checkBroadcast( "", "hub.expression.changed", {}, __LINE__ );

    deleteScene( sceneId );
    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ScenesExpressionsTests, variableExpressionScaleNotFoundError ) /* 6.5 */ {
    const auto expressionName = "variableExpressionList";
    const Json value = Json{ { "value", 1 }, { "scale", "" } };
    bool isExpectedBroadcast = true;
    Json command = makeVariableExpressionSetCommand( expressionName, value, "temperature", {}, isExpectedBroadcast );
    auto answer = nma().sendtoHubAndGetAnswer( command );
    checkHasError( answer, __FILE__, __LINE__ );

    Json errorData;
    EXPECT_TRUE( json_get( answer, { "error", "data" }, errorData ) );
    EXPECT_EQ( json_get_str( errorData ), "rpc.params.scale.not.found" );
}

TEST_F( ScenesExpressionsTests, variableExpressionInavlidScaleError ) /* 6.6 */ {
    const auto expressionName = "variableExpressionList";
    const Json value = Json{ { "value", 1 }, { "scale", CoreD::Model::ValueScales::Ampere } };
    bool isExpectedBroadcast = true;
    Json command = makeVariableExpressionSetCommand( expressionName, value, "temperature", {}, isExpectedBroadcast );
    auto answer = nma().sendtoHubAndGetAnswer( command );
    checkHasError( answer, __FILE__, __LINE__ );

    Json errorData;
    EXPECT_TRUE( json_get( answer, { "error", "data" }, errorData ) );
    EXPECT_EQ( json_get_str( errorData ), "rpc.params.scale.not.found" );
}

TEST_F( ScenesExpressionsTests, saveGlobalVariableHttpRequestNonExistent_T7_1 ) /* 7.1 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const std::string expressionName = "variableDoesNotExist";

    Tests::HttpServer& server = getHttpServer();

    auto actionBlock = replaceTokensInString( sendHttpRequestSaveResultThenBlock,
                                              { { "__URL__", makeUrl( "/reflect" ) },
                                                { "__USER__", "" },
                                                { "__PASSWORD__", "" },
                                                { "__REQUEST_TYPE__", "GET" },
                                                { "__CONTENT_TYPE__", "text/plain" },
                                                { "__CONTENT__", "" },
                                                { "__HEADERS__", "" },
                                                { "__SKIP_SECURITY__", "true" } } );

    bool isExpectedBroadcast = true;
    scenesVariableExpressionSetType< int >(
        expressionName, 1, "hub.expression.added", "int", {}, isExpectedBroadcast, __LINE__ );

    auto request = replaceTokens( createSceneRequest,
                                  { { "__NAME__", "HttpRequestSave" },
                                    { "__WHEN_BLOCKS__", "" },
                                    { "__THEN_BLOCKS__", std::move( actionBlock ) } } );
    SceneID sceneId = createScene( request );
    runSceneManually( sceneId, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "failed", __LINE__ );

    deleteSceneExpression( expressionName, __LINE__ );
    deleteScene( sceneId );
}

TEST_F( ScenesExpressionsTests, saveGlobalVariableHttpRequest_T7_2 ) /* 7.2 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const std::string expressionName = "setGlobalVariableAction";

    Tests::HttpServer& server = getHttpServer();

    auto actionBlock = replaceTokensInString( sendHttpRequestSaveResultThenBlock,
                                              { { "__URL__", makeUrl( "/reflect" ) },
                                                { "__USER__", "" },
                                                { "__PASSWORD__", "" },
                                                { "__REQUEST_TYPE__", "GET" },
                                                { "__CONTENT_TYPE__", "text/plain" },
                                                { "__CONTENT__", "" },
                                                { "__HEADERS__", "" },
                                                { "__SKIP_SECURITY__", "true" },
                                                { "__GLOBAL_VARIABLE_NAME__", expressionName } } );

    bool isExpectedBroadcast = true;
    scenesVariableExpressionSetType< int >(
        expressionName, 1, "hub.expression.added", "int", {}, isExpectedBroadcast, __LINE__ );

    auto request = replaceTokens( createSceneRequest,
                                  { { "__NAME__", "HttpRequestSave" },
                                    { "__WHEN_BLOCKS__", "" },
                                    { "__THEN_BLOCKS__", std::move( actionBlock ) } } );
    SceneID sceneId = createScene( request );
    runSceneManually( sceneId, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    checkBroadcast( "", "hub.expression.changed", { { "name", expressionName } }, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    Json answer;
    JsonArray expressions;
    runExpressionsList( answer, false );

    LOGGER_TRACE( cc::j( answer ) );
    checkNoError( answer, __FILE__, __LINE__ );

    checkExpressions( answer, expressions, __LINE__ );
    auto metadata = JSON( "saveResult" << JSON( "code" << 200 << "connection"
                                                       << "close"
                                                       << "content-type"
                                                       << "application/json" ) );
    Log::debug( "metadata: " + cc::j( metadata ) );

    checkExpressionByName( expressions, expressionName, { { "metadata", metadata } }, __LINE__ );

    deleteSceneExpression( expressionName, __LINE__ );
    deleteScene( sceneId );
}

TEST_F( ScenesExpressionsTests, saveGlobalVariableHttpRequestWithError_T7_3 ) /* 7.3 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const std::string expressionName = "setGlobalVarWithError";

    Tests::HttpServer& server = getHttpServer();

    bool isExpectedBroadcast = true;
    scenesVariableExpressionSetType< int >(
        expressionName, 1, "hub.expression.added", "int", {}, isExpectedBroadcast, __LINE__ );

    Json answer, expressions;
    runExpressionsList( answer, false );
    checkExpressions( answer, expressions, __LINE__ );

    std::string actionBlock = replaceTokensInString( sendHttpRequestSaveResultThenBlock,
                                                     { { "__URL__", makeUrl( "/nonexistent_endpoint" ) },
                                                       { "__USER__", "" },
                                                       { "__PASSWORD__", "" },
                                                       { "__REQUEST_TYPE__", "GET" },
                                                       { "__CONTENT_TYPE__", "text/plain" },
                                                       { "__CONTENT__", "" },
                                                       { "__HEADERS__", "" },
                                                       { "__SKIP_SECURITY__", "true" },
                                                       { "__GLOBAL_VARIABLE_NAME__", expressionName } } );

    Json request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "HttpRequest" }, { "__WHEN_BLOCKS__", "" }, { "__THEN_BLOCKS__", std::move( actionBlock ) } } );
    std::string sceneId = createScene( request );

    runExpressionsList( answer, false );
    checkNoError( answer, __FILE__, __LINE__ );

    runSceneManually( sceneId, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    checkBroadcast( "", "hub.expression.changed", { { "name", expressionName } }, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "failed", __LINE__ );

    runExpressionsList( answer, false );
    checkNoError( answer, __FILE__, __LINE__ );

    checkExpressions( answer, expressions, __LINE__ );

    auto error = JSON( "saveResult" << JSON( "code" << 5 << "text"
                                                    << "The requested URL returned error: 404 Not Found" ) );
    checkExpressionByName( expressions, expressionName, { { "error", error } }, __LINE__ );
    deleteSceneExpression( expressionName, __LINE__ );
    deleteScene( sceneId );
}

TEST_F( ScenesExpressionsTests, saveGlobalVariableHttpRequest_T7_4 ) /* 7.4 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const std::string expressionName1 = "var1";
    const std::string expressionName2 = "var2";
    const std::string UrlPrefix = "/reflect?name=";

    Tests::HttpServer& server = getHttpServer();

    auto makeActionBlock = [=]( const std::string& expressionName ) -> std::string {
        const auto url = UrlPrefix + expressionName;
        return replaceTokensInString( sendHttpRequestSaveResultThenBlock,
                                      { { "__URL__", makeUrl( url.c_str() ) },
                                        { "__USER__", "" },
                                        { "__PASSWORD__", "" },
                                        { "__REQUEST_TYPE__", "GET" },
                                        { "__CONTENT_TYPE__", "text/plain" },
                                        { "__CONTENT__", "" },
                                        { "__HEADERS__", "" },
                                        { "__SKIP_SECURITY__", "true" },
                                        { "__GLOBAL_VARIABLE_NAME__", expressionName } } );
    };

    auto actionBlock1 = makeActionBlock( expressionName1 );
    auto actionBlock2 = makeActionBlock( expressionName2 );

    bool isExpectedBroadcast = true;
    scenesVariableExpressionSetType< int >(
        expressionName1, 1, "hub.expression.added", "int", {}, isExpectedBroadcast, __LINE__ );

    scenesVariableExpressionSetType< int >(
        expressionName2, 1, "hub.expression.added", "int", {}, isExpectedBroadcast, __LINE__ );

    auto request = replaceTokens( createSceneRequest,
                                  { { "__NAME__", "HttpRequestSave" },
                                    { "__WHEN_BLOCKS__", "" },
                                    { "__THEN_BLOCKS__", std::move( actionBlock1 + "," + actionBlock2 ) } } );

    SceneID sceneId = createScene( request );
    runSceneManually( sceneId, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    checkBroadcast( "", "hub.expression.changed", {}, __LINE__ );
    checkBroadcast( "", "hub.expression.changed", {}, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    Json answer;
    JsonArray expressions;
    runExpressionsList( answer, false );
    checkNoError( answer, __FILE__, __LINE__ );
    checkExpressions( answer, expressions, __LINE__ );

    auto expressionValue = getExpressionValueByName( expressions, expressionName1, __LINE__ );
    EXPECT_TRUE( expressionValue.get< std::string >().find( expressionName1 ) != std::string::npos );
    expressionValue = getExpressionValueByName( expressions, expressionName2, __LINE__ );
    EXPECT_TRUE( expressionValue.get< std::string >().find( expressionName2 ) != std::string::npos );

    deleteSceneExpression( expressionName1, __LINE__ );
    deleteSceneExpression( expressionName2, __LINE__ );

    deleteScene( sceneId );
}

TEST_F( ScenesExpressionsTests, saveHttpRequestResultTwoRequestsDirectOrder_T7_5 ) /* 7.5 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const std::string expressionName1 = "var1";
    const std::string expressionName2 = "var2";
    const std::string urlPrefix1 = "/reflect?name=";
    const std::string urlPrefix2 = "/reflect?__delayed=2&name=";

    Tests::HttpServer& server = getHttpServer();

    auto makeActionBlock = [this]( const std::string& urlPrefix, const std::string& expressionName ) -> std::string {
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
                                        { "__GLOBAL_VARIABLE_NAME__", expressionName } } );
    };

    auto actionBlock1 = makeActionBlock( urlPrefix1, expressionName1 );
    auto actionBlock2 = makeActionBlock( urlPrefix2, expressionName2 );

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
    checkBroadcast( "", "hub.expression.changed", { { "name", expressionName1 } }, __LINE__ );
    checkBroadcast( "", "hub.expression.changed", { { "name", expressionName2 } }, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    Json answer;
    JsonArray expressions;
    runExpressionsList( answer, false );
    checkNoError( answer, __FILE__, __LINE__ );
    checkExpressions( answer, expressions, __LINE__ );

    auto expressionValue = getExpressionValueByName( expressions, expressionName1, __LINE__ );
    EXPECT_TRUE( expressionValue.get< std::string >().find( expressionName1 ) != std::string::npos );
    expressionValue = getExpressionValueByName( expressions, expressionName2, __LINE__ );
    EXPECT_TRUE( expressionValue.get< std::string >().find( expressionName2 ) != std::string::npos );

    deleteSceneExpression( expressionName1, __LINE__ );
    deleteSceneExpression( expressionName2, __LINE__ );

    deleteScene( sceneId );
}

TEST_F( ScenesExpressionsTests, saveHttpRequestResultTwoRequestsInverseOrder_T7_6 ) /* 7.6 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const std::string expressionName1 = "var1";
    const std::string expressionName2 = "var2";
    const std::string urlPrefix1 = "/reflect?__delayed=2000&name=";
    const std::string urlPrefix2 = "/reflect?name=";

    Tests::HttpServer& server = getHttpServer();

    auto makeActionBlock = [this]( const std::string& urlPrefix, const std::string& expressionName ) -> std::string {
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
                                        { "__GLOBAL_VARIABLE_NAME__", expressionName } } );
    };

    auto actionBlock1 = makeActionBlock( urlPrefix1, expressionName1 );
    auto actionBlock2 = makeActionBlock( urlPrefix2, expressionName2 );

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
    checkBroadcast( "", "hub.expression.changed", { { "name", expressionName2 } }, __LINE__ );
    checkBroadcast( "", "hub.expression.changed", { { "name", expressionName1 } }, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    Json answer;
    JsonArray expressions;
    runExpressionsList( answer, false );
    checkNoError( answer, __FILE__, __LINE__ );
    checkExpressions( answer, expressions, __LINE__ );

    auto expressionValue = getExpressionValueByName( expressions, expressionName1, __LINE__ );
    EXPECT_TRUE( expressionValue.get< std::string >().find( expressionName1 ) != std::string::npos );
    expressionValue = getExpressionValueByName( expressions, expressionName2, __LINE__ );
    EXPECT_TRUE( expressionValue.get< std::string >().find( expressionName2 ) != std::string::npos );

    deleteSceneExpression( expressionName1, __LINE__ );
    deleteSceneExpression( expressionName2, __LINE__ );

    deleteScene( sceneId );
}

TEST_F( ScenesExpressionsTests, saveGlobalVariableHttpRequestErrorToSetVariableWithSaveTypeInResult ) {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const std::string expressionName = "setGlobalVarWithError";

    Tests::HttpServer& server = getHttpServer();

    bool isExpectedBroadcast = true;
    std::string oldType = "int";
    auto oldMetadata = JSON( "a"
                             << "b" );

    scenesVariableExpressionSetType< int >(
        expressionName, 1, "hub.expression.added", oldType, oldMetadata, isExpectedBroadcast, __LINE__ );

    Json answer, expressions;
    runExpressionsList( answer, false );
    checkExpressions( answer, expressions, __LINE__ );

    std::string actionBlock = replaceTokensInString( sendHttpRequestSaveResultThenBlock,
                                                     { { "__URL__", makeUrl( "/nonexistent_endpoint" ) },
                                                       { "__USER__", "" },
                                                       { "__PASSWORD__", "" },
                                                       { "__REQUEST_TYPE__", "GET" },
                                                       { "__CONTENT_TYPE__", "text/plain" },
                                                       { "__CONTENT__", "" },
                                                       { "__HEADERS__", "" },
                                                       { "__SKIP_SECURITY__", "true" },
                                                       { "__GLOBAL_VARIABLE_NAME__", expressionName } } );

    Json request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "HttpRequest" }, { "__WHEN_BLOCKS__", "" }, { "__THEN_BLOCKS__", std::move( actionBlock ) } } );
    std::string sceneId = createScene( request );

    runExpressionsList( answer, false );
    checkNoError( answer, __FILE__, __LINE__ );

    runSceneManually( sceneId, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    checkBroadcast( "", "hub.expression.changed", { { "name", expressionName } }, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "failed", __LINE__ );

    runExpressionsList( answer, false );
    checkNoError( answer, __FILE__, __LINE__ );

    checkExpressions( answer, expressions, __LINE__ );

    checkExpressionByName( expressions,
                           expressionName,
                           {
                               { "valueType", oldType },
                               { "metadata", oldMetadata },
                               { Resources::CheckBroadcast::HasNot, "value" },
                               { Resources::CheckBroadcast::Has, "error" },
                           },
                           __LINE__ );

    deleteSceneExpression( expressionName, __LINE__ );
    deleteScene( sceneId );
}

TEST_F( ScenesExpressionsTests, CloudAPISaveResultThenBlockFail_8_1 ) /* 8.1 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const std::string expressionName = "setGlobalVarActionCloudFail";
    bool isExpectedBroadcast = true;
    scenesVariableExpressionSetType< int >(
        expressionName, 1, "hub.expression.added", "int", {}, isExpectedBroadcast, __LINE__ );

    auto thenBlock = replaceTokensInString( HubPlatform::Tests::cloudAPIActionSaveResultThenBlock,
                                            { { "__CALL__", "some_api_call" } } );
    auto request = replaceTokensInString(
        createSceneRequest,
        { { "__NAME__", "cloudAPIAction" }, { "__WHEN_BLOCKS__", "" }, { "__THEN_BLOCKS__", thenBlock } } );
    int cloudCallsCounter = 0;

    auto goodCloudResponse = []( rpc::Message& rpcMessage ) {
        JsonObject& joAnswer = rpcMessage.getAnswer();
        JsonObject& joMessage = rpcMessage.getMessage();
        joAnswer[ "method" ] = json_get_str( joMessage, "method" );
        joAnswer[ "result" ] = JSON_EMPTY_OBJ;
        joAnswer[ "error" ] = nullptr;
        JsonObject params;
        json_get_obj( joMessage, "params", params );
    };

    nma().setCloudMethod( "cloud.some_api_call", goodCloudResponse );

    JsonObject CloudAPI = JsonObject::parse( request );
    SceneID sceneId = createScene( CloudAPI );
    runSceneManually( sceneId, __LINE__ );

    testRunProgressBroadcast( CloudAPI, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( CloudAPI, sceneId, "failed", __LINE__ );

    deleteSceneExpression( expressionName, __LINE__ );
    deleteScene( sceneId );
}

TEST_F( ScenesExpressionsTests, CloudAPISaveResultThenBlock_8_2 ) /* 8.2 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const std::string expressionName = "setGlobalVariableActionCloud";
    bool isExpectedBroadcast = true;
    scenesVariableExpressionSetType< int >(
        expressionName, 1, "hub.expression.added", "int", {}, isExpectedBroadcast, __LINE__ );

    auto thenBlock = replaceTokensInString( HubPlatform::Tests::cloudAPIActionSaveResultThenBlock,
                                            { { "__CALL__", "some_api_call" } } );

    auto request = replaceTokensInString(
        createSceneRequest,
        { { "__NAME__", "cloudAPIAction" }, { "__WHEN_BLOCKS__", "" }, { "__THEN_BLOCKS__", thenBlock } } );

    auto testValue = JSON( "key"
                           << "value" );
    auto goodCloudResponse = [testValue]( rpc::Message& rpcMessage ) {
        JsonObject& joAnswer = rpcMessage.getAnswer();
        JsonObject& joMessage = rpcMessage.getMessage();
        joAnswer[ "method" ] = json_get_str( joMessage, "method" );
        joAnswer[ "result" ] = testValue;
        joAnswer[ "error" ] = nullptr;

        JsonObject params;
        json_get_obj( joMessage, "params", params );
    };

    nma().setCloudMethod( "cloud.some_api_call", goodCloudResponse );

    JsonObject CloudAPI = JsonObject::parse( request );
    SceneID sceneId = createScene( CloudAPI );
    runSceneManually( sceneId, __LINE__ );

    testRunProgressBroadcast( CloudAPI, sceneId, "started", __LINE__ );
    checkBroadcast( "", "hub.expression.changed", {}, __LINE__ );
    testRunProgressBroadcast( CloudAPI, sceneId, "finished", __LINE__ );

    Json answer;
    JsonArray expressions;
    runExpressionsList( answer, false );
    checkNoError( answer, __FILE__, __LINE__ );

    checkExpressions( answer, expressions, __LINE__ );
    checkExpressionByName( expressions, expressionName, { { "value", testValue } }, __LINE__ );

    deleteSceneExpression( expressionName, __LINE__ );
    deleteScene( sceneId );
}

TEST_F( ScenesExpressionsTests, CloudAPISaveResultThenBlockError_8_3 ) /* 8.3 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const std::string expressionName = "setGlobalVariableActionCloud";
    bool isExpectedBroadcast = true;
    scenesVariableExpressionSetType< std::string >(
        expressionName, "string value", "hub.expression.added", "string", {}, isExpectedBroadcast, __LINE__ );

    auto thenBlock = replaceTokensInString( HubPlatform::Tests::cloudAPIActionSaveResultThenBlock,
                                            { { "__CALL__", "some_api_call" } } );
    auto request = replaceTokensInString(
        createSceneRequest,
        { { "__NAME__", "cloudAPIAction" }, { "__WHEN_BLOCKS__", "" }, { "__THEN_BLOCKS__", thenBlock } } );
    auto testValue = JSON( "code" << 12 << "data"
                                  << "cloud.error.invalid_command" );

    auto goodCloudResponse = [testValue]( rpc::Message& rpcMessage ) {
        JsonObject& joAnswer = rpcMessage.getAnswer();
        JsonObject& joMessage = rpcMessage.getMessage();
        joAnswer[ "method" ] = json_get_str( joMessage, "method" );
        joAnswer[ "result" ] = nullptr;
        joAnswer[ "error" ] = testValue;

        JsonObject params;
        json_get_obj( joMessage, "params", params );
    };

    nma().setCloudMethod( "cloud.some_api_call", goodCloudResponse );

    JsonObject CloudAPI = JsonObject::parse( request );
    SceneID sceneId = createScene( CloudAPI );
    runSceneManually( sceneId, __LINE__ );

    testRunProgressBroadcast( CloudAPI, sceneId, "started", __LINE__ );
    checkBroadcast( "", "hub.expression.changed", {}, __LINE__ );
    testRunProgressBroadcast( CloudAPI, sceneId, "failed", __LINE__ );

    deleteSceneExpression( expressionName, __LINE__ );
    deleteScene( sceneId );
}

TEST_F( ScenesExpressionsTests, setExpressionWithBigData_9_1 ) {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );

    const std::string expressionSourceName = "expressionSource1";

    auto code = getBigString();
    Json command = makeExpressionSetCommand( expressionSourceName, code, {}, {}, "int", {}, false, false );
    auto answer = nma().sendtoHubAndGetAnswer( command );
    checkHasError( answer, __FILE__, __LINE__ );
    auto error = answer[ "error" ];
    EXPECT_EQ( json_get_int( error, "code" ), -32603 );
    EXPECT_EQ( json_get_str( error, "data" ), "bus.message.too.long" );
    EXPECT_EQ( json_get_str( error, "message" ), "Bus message is too long" );
}

TEST_F( ScenesExpressionsTests, createSceneWithBigData_T9_2 ) /* 9.2 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const std::string expressionName1 = "var1";
    const std::string UrlPrefix = "/reflect?name=";

    Tests::HttpServer& server = getHttpServer();

    auto makeActionBlock = [=]( const std::string& expressionName ) -> std::string {
        const auto url = UrlPrefix + expressionName;
        return replaceTokensInString( sendHttpRequestSaveResultThenBlock,
                                      { { "__URL__", makeUrl( url.c_str() ) },
                                        { "__USER__", "" },
                                        { "__PASSWORD__", "" },
                                        { "__REQUEST_TYPE__", "GET" },
                                        { "__CONTENT_TYPE__", "text/plain" },
                                        { "__CONTENT__", getBigString() },
                                        { "__HEADERS__", "" },
                                        { "__SKIP_SECURITY__", "true" },
                                        { "__GLOBAL_VARIABLE_NAME__", expressionName } } );
    };

    auto actionBlock = makeActionBlock( expressionName1 );

    auto request = replaceTokens( createSceneRequest,
                                  { { "__NAME__", "HttpRequestSave" },
                                    { "__WHEN_BLOCKS__", "" },
                                    { "__THEN_BLOCKS__", std::move( actionBlock ) } } );

    auto answer = nma().sendtoHubAndGetAnswer( request );
    checkHasError( answer, __FILE__, __LINE__ );
    auto error = answer[ "error" ];
    EXPECT_EQ( json_get_int( error, "code" ), -32603 );
    EXPECT_EQ( json_get_str( error, "data" ), "bus.message.too.long" );
    EXPECT_EQ( json_get_str( error, "message" ), "Bus message is too long" );
}

}  // namespace Tests
}  // namespace HubPlatform
