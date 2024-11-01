#include "fixtures/scenes_expressions_fixture.h"
#include "common_test_resources/resources/scene_manager_resources.h"
#include "cored/remote_interface/cored_remote_interface.h"
#include "database_client/dbqueryparser.h"
#include <gtest/gtest.h>
#include "system_utils/find_free_local_port.h"
#include "tiny/tiny_consolecolors.h"

/*

TESTPLAN

DONE    1. toggle expression bool value test
DONE    2. toggle expression bool value test
DONE    3. toggle expression intValue test
DONE    4. toggle expression name test failed
DONE    5. toggle item intValue test
DONE    6. toggle item boolValue test
DONE    7. toggle item intValue test failed
DONE    8. toggle itemId test failed

*/

namespace HubPlatform {
namespace Tests {

class ToggleValueTests : public ScenesExpressionsFixture {
public:
    ToggleValueTests();
    void SetUp() override;
    void TearDown() override;
    static void SetUpTestCase();

    JsonObject getItemValue( const std::string itemId );

protected:
    CoreDRemoteInterface::CoreDInterface& m_remote;
};

ToggleValueTests::ToggleValueTests()
: m_remote( makeRemote( nma() ) ) {}

void ToggleValueTests::SetUp() {
    ScenesExpressionsFixture::SetUp();
}

void ToggleValueTests::TearDown() {
    ScenesExpressionsFixture::TearDown();
}

void ToggleValueTests::SetUpTestCase() {}

JsonObject ToggleValueTests::getItemValue( const std::string itemId ) {
    auto item = m_remote.getItem( itemId );
    JsonObject value;
    json_get( item, "value", value );

    return value;
}

TEST_F( ToggleValueTests, toggleExpressionBoolValueTest ) /*1*/ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const std::string expressionName = "toggleValueTest";

    bool isExpectedBroadcast = true;
    scenesVariableExpressionSetType< bool >(
        expressionName, true, "hub.expression.added", "bool", {}, isExpectedBroadcast, __LINE__ );

    Json answer, expressions;
    runExpressionsList( answer, false );
    checkExpressions( answer, expressions, __LINE__ );

    std::string actionBlock =
        replaceTokensInString( toggleExpressionValueThenBlock, { { "__VALUE__", expressionName } } );

    Json request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "ToggleValue" }, { "__WHEN_BLOCKS__", "" }, { "__THEN_BLOCKS__", std::move( actionBlock ) } } );

    std::string sceneId = createScene( request );

    runSceneManually( sceneId, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    checkBroadcast( "", "hub.expression.changed", {}, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
    runExpressionsList( answer, false );
    checkNoError( answer, __FILE__, __LINE__ );
    checkExpressions( answer, expressions, __LINE__ );
    checkExpressionByName( expressions, expressionName, { { "value", false } }, __LINE__ );

    runSceneManually( sceneId, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    checkBroadcast( "", "hub.expression.changed", {}, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
    runExpressionsList( answer, false );
    checkNoError( answer, __FILE__, __LINE__ );
    checkExpressions( answer, expressions, __LINE__ );
    checkExpressionByName( expressions, expressionName, { { "value", true } }, __LINE__ );

    deleteSceneExpression( expressionName, __LINE__ );
    deleteScene( sceneId );
}

TEST_F( ToggleValueTests, toggleExpressionIntValueTest ) /*2*/ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const std::string expressionName = "toggleValueTest";

    bool isExpectedBroadcast = true;
    scenesVariableExpressionSetType< int >(
        expressionName, 0, "hub.expression.added", "int", {}, isExpectedBroadcast, __LINE__ );

    Json answer, expressions;
    runExpressionsList( answer, false );
    checkExpressions( answer, expressions, __LINE__ );

    std::string actionBlock =
        replaceTokensInString( toggleExpressionValueThenBlock, { { "__VALUE__", expressionName } } );
    Json request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "ToggleValue" }, { "__WHEN_BLOCKS__", "" }, { "__THEN_BLOCKS__", std::move( actionBlock ) } } );

    std::string sceneId = createScene( request );

    runSceneManually( sceneId, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    checkBroadcast( "", "hub.expression.changed", {}, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
    runExpressionsList( answer, false );
    checkNoError( answer, __FILE__, __LINE__ );
    checkExpressions( answer, expressions, __LINE__ );
    checkExpressionByName( expressions, expressionName, { { "value", 1 } }, __LINE__ );

    runSceneManually( sceneId, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    checkBroadcast( "", "hub.expression.changed", {}, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
    runExpressionsList( answer, false );
    checkNoError( answer, __FILE__, __LINE__ );
    checkExpressions( answer, expressions, __LINE__ );
    checkExpressionByName( expressions, expressionName, { { "value", 0 } }, __LINE__ );

    deleteSceneExpression( expressionName, __LINE__ );
    deleteScene( sceneId );
}

TEST_F( ToggleValueTests, toggleExpressionIntValueTestFailed ) /*3*/ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const std::string expressionName = "toggleValueTest";

    bool isExpectedBroadcast = true;
    scenesVariableExpressionSetType< std::string >(
        expressionName, "test", "hub.expression.added", "string", {}, isExpectedBroadcast, __LINE__ );

    Json answer, expressions;
    runExpressionsList( answer, false );
    checkExpressions( answer, expressions, __LINE__ );

    std::string actionBlock =
        replaceTokensInString( toggleExpressionValueThenBlock, { { "__VALUE__", expressionName } } );
    Json request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "ToggleValue" }, { "__WHEN_BLOCKS__", "" }, { "__THEN_BLOCKS__", std::move( actionBlock ) } } );

    answer = nma().sendtoHubAndGetAnswer( request );
    checkHasError( answer, __FILE__, __LINE__ );
    Json errorData;
    EXPECT_TRUE( json_get( answer, { "error", "data" }, errorData ) ) << "Should end with error";
    EXPECT_EQ( json_get_str( errorData ), "scenes.action.type.mismatch" );

    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ToggleValueTests, toggleExpressionNameTestFailed ) /*4*/ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const std::string expressionName = "toggleValueTest";

    bool isExpectedBroadcast = true;
    scenesVariableExpressionSetType< std::string >(
        expressionName, "test", "hub.expression.added", "string", {}, isExpectedBroadcast, __LINE__ );

    Json answer, expressions;
    runExpressionsList( answer, false );
    checkExpressions( answer, expressions, __LINE__ );

    std::string actionBlock =
        replaceTokensInString( toggleExpressionValueThenBlock, { { "__VALUE__", "expressionName" } } );
    Json request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "ToggleValue" }, { "__WHEN_BLOCKS__", "" }, { "__THEN_BLOCKS__", std::move( actionBlock ) } } );

    answer = nma().sendtoHubAndGetAnswer( request );
    checkHasError( answer, __FILE__, __LINE__ );
    Json errorData;
    EXPECT_TRUE( json_get( answer, { "error", "data" }, errorData ) ) << "Should end with error";
    EXPECT_EQ( json_get_str( errorData ), "scenes.expression.does.not.exist" );

    deleteSceneExpression( expressionName, __LINE__ );
}

TEST_F( ToggleValueTests, toggleItemIntValueTest ) /*5*/ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );

    auto deviceId = makeDeviceSiren( __LINE__ );
    using namespace CoreD::Model;
    const std::string itemName = "sound_volume";

    Item item = makeIntItem( deviceId, itemName, 0 );
    auto itemId = addItem( m_remote, item, __LINE__, nullptr );

    std::string actionBlock = replaceTokensInString( toggleItemValueThenBlock, { { "__VALUE__", itemId } } );

    Json request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "ToggleValue" }, { "__WHEN_BLOCKS__", "" }, { "__THEN_BLOCKS__", std::move( actionBlock ) } } );

    std::string sceneId = createScene( request );
    runSceneManually( sceneId, __LINE__ );

    EXPECT_TRUE( waitAndCheckRunProgressBroadcast( request, sceneId, "started", __LINE__ ) );
    EXPECT_TRUE( waitAndCheckRunProgressBroadcast( request, sceneId, "finished", __LINE__ ) );
    EXPECT_TRUE( waitAndCheckItemUpdateBroadcast( itemId, 1, __LINE__ ) );
    EXPECT_EQ( getItemValue( itemId ), 1 );

    runSceneManually( sceneId, __LINE__ );

    EXPECT_TRUE( waitAndCheckRunProgressBroadcast( request, sceneId, "started", __LINE__ ) );
    EXPECT_TRUE( waitAndCheckRunProgressBroadcast( request, sceneId, "finished", __LINE__ ) );
    EXPECT_TRUE( waitAndCheckItemUpdateBroadcast( itemId, 0, __LINE__ ) );
    EXPECT_EQ( getItemValue( itemId ), 0 );

    deleteScene( sceneId );
}

TEST_F( ToggleValueTests, toggleItemBoolValueTest ) /*6*/ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );

    auto deviceId = makeDeviceSiren( __LINE__ );
    using namespace CoreD::Model;
    const std::string itemName = "sound_volume";

    Item item = makeBoolItem( deviceId, itemName, false );
    auto itemId = addItem( m_remote, item, __LINE__, nullptr );
    std::string actionBlock = replaceTokensInString( toggleItemValueThenBlock, { { "__VALUE__", itemId } } );
    Json request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "ToggleValue" }, { "__WHEN_BLOCKS__", "" }, { "__THEN_BLOCKS__", std::move( actionBlock ) } } );

    auto requestStr = request.dump();
    std::string sceneId = createScene( request );

    runSceneManually( sceneId, __LINE__ );

    EXPECT_TRUE( waitAndCheckRunProgressBroadcast( request, sceneId, "started", __LINE__ ) );
    EXPECT_TRUE( waitAndCheckRunProgressBroadcast( request, sceneId, "finished", __LINE__ ) );
    EXPECT_TRUE( waitAndCheckItemUpdateBroadcast( itemId, __LINE__ ) );
    EXPECT_EQ( getItemValue( itemId ), true );

    runSceneManually( sceneId, __LINE__ );
    EXPECT_TRUE( waitAndCheckRunProgressBroadcast( request, sceneId, "started", __LINE__ ) );
    EXPECT_TRUE( waitAndCheckRunProgressBroadcast( request, sceneId, "finished", __LINE__ ) );
    EXPECT_TRUE( waitAndCheckItemUpdateBroadcast( itemId, __LINE__ ) );
    EXPECT_EQ( getItemValue( itemId ), false );

    deleteScene( sceneId );
}

TEST_F( ToggleValueTests, toggleItemIntValueTestFailed ) /*7*/ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );

    auto deviceId = makeDeviceSiren( __LINE__ );
    using namespace CoreD::Model;
    const std::string itemName = "sound_volume";

    Item item = makeStringItem( deviceId, itemName, "test" );
    auto itemId = addItem( m_remote, item, __LINE__, nullptr );

    std::string actionBlock = replaceTokensInString( toggleItemValueThenBlock, { { "__VALUE__", itemId } } );

    Json request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "ToggleValue" }, { "__WHEN_BLOCKS__", "" }, { "__THEN_BLOCKS__", std::move( actionBlock ) } } );

    auto answer = nma().sendtoHubAndGetAnswer( request );
    checkHasError( answer, __FILE__, __LINE__ );
    Json errorData;
    EXPECT_TRUE( json_get( answer, { "error", "data" }, errorData ) ) << "Should end with error";
    EXPECT_EQ( json_get_str( errorData ), "scenes.action.type.mismatch" );
}

TEST_F( ToggleValueTests, toggleItemIdTestFailed ) /*8*/ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    std::string actionBlock = replaceTokensInString( toggleItemValueThenBlock, { { "__VALUE__", "itemId" } } );

    Json request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "ToggleValue" }, { "__WHEN_BLOCKS__", "" }, { "__THEN_BLOCKS__", std::move( actionBlock ) } } );

    auto answer = nma().sendtoHubAndGetAnswer( request );
    checkHasError( answer, __FILE__, __LINE__ );
    Json errorData;
    EXPECT_TRUE( json_get( answer, { "error", "data" }, errorData ) ) << "Should end with error";
    EXPECT_EQ( json_get_str( errorData ), "scenes.item.does.not.exist" );
}

}  // namespace Tests
}  // namespace HubPlatform
