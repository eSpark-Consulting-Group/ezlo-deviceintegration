#include "common_test_resources/fixtures/scene_manager_tests_fixture.h"
#include "common_test_resources/resources/scene_manager_resources.h"
#include "common_test_resources/resources/cored_resources.h"

#include "cored/model/values/value_factory.h"
#include <gtest/gtest.h>
#include "tiny/tiny_consolecolors.h"
#include "cored/remote_interface/cored_remote_interface.h"

namespace HubPlatform {
namespace Tests {

/*

TESTPLAN

DONE    1 CompareNumbersEvent
DONE        1.1 Create scene with global restriction block
DONE    2 CompareStringsEvent
DONE        2.1 Create scene with global restriction block
DONE    3 IsItemStateEvent
DONE        3.1 Create scene with global restriction block
DONE    4 StringOperationEvent
DONE        4.1 Create scene with global restriction block
DONE    5 InArrayEvent
DONE        4.1 Create scene with global restriction block
DONE    6 CompareNumberRangeEvent
DONE        6.1 Create scene with global restriction block
DONE    7 Create Scene Error
DONE        7.1 Create scene with global restriction block
DONE    8 Scene State
DONE        8.1 Create scene with global restriction block
DONE    9 AND Operator
DONE        9.1 Create scene with global restriction block
DONE    10 OR Operator
DONE        10.1 Create scene with global restriction block
DONE    11 NOT Operator
DONE        11.1 Create scene with global restriction block
DONE    12 Initial State test
DONE        12.1 Create scene with global restriction block
DONE    13 AND Operator initial state
DONE        13.1 Create scene with global restriction block
DONE    14 Lua scene module
DONE        14.1 Checking block state through Lua module
*/

class SceneGlobalRestrictionTests : public SceneManagerTestsFixture {
public:
    static void SetUpTestCase();

    void SetUp() override;

    void TearDown() override;

    void removeDevice( const std::string& deviceId, int codeLine );
};

void SceneGlobalRestrictionTests::SetUpTestCase() {}

void SceneGlobalRestrictionTests::SetUp() {
    SceneManagerTestsFixture::SetUp();
    callingFile( __FILE__ );
}

void SceneGlobalRestrictionTests::TearDown() {
    auto ignoreAllPresentBroadcasts = []( const JsonObject& br ) {
        Log::error( cc::error( "# SceneGlobalRestrictionTests::TearDown: Not handled broadcasts: " ) + cc::j( br ) );
        return true;
    };
    ignoreBroadcasts( ignoreAllPresentBroadcasts, __FILE__, __LINE__, std::chrono::milliseconds( 100 ) );

    SceneManagerTestsFixture::TearDown();
}

void SceneGlobalRestrictionTests::removeDevice( const std::string& deviceId, int codeLine ) {
    auto& remoteInterface = makeRemote( nma() );
    auto itemsNumber = remoteInterface.getItemsByDeviceId( deviceId ).size();
    CoreNmaTest::removeDevice( remoteInterface, deviceId, itemsNumber, 0, codeLine );
}

TEST_F( SceneGlobalRestrictionTests, compareNumbers_exactMatch ) {  // 1.1
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId = addDevice( coreRemoteInterface,
                               "testDevice1",
                               "smoke",
                               "134_3_24",
                               "security_sensor",
                               "smoke",
                               false /*batteryPowered*/,
                               __LINE__ );

    auto itemId1 =
        addItem( coreRemoteInterface, deviceId, "testItem1", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );
    auto itemId2 =
        addItem( coreRemoteInterface, deviceId, "testItem2", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );
    auto itemId3 =
        addItem( coreRemoteInterface, deviceId, "testItem3", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );
    auto itemId4 =
        addItem( coreRemoteInterface, deviceId, "testItem3", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );
    auto itemId5 =
        addItem( coreRemoteInterface, deviceId, "testItem3", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );

    auto whenBlock = replaceTokensInString( compareNumbersWhenBlock,
                                            { { "__SOURCE_NAME__", "item" },
                                              { "__SOURCE_TYPE__", "item" },
                                              { "__SOURCE_VALUE__", "\"" + itemId1 + "\"" },
                                              { "__COMPARATOR__", ">=" },
                                              { "__TYPE__", "int" },
                                              { "__VALUE__", "10" } } );

    auto block = replaceTokensInString( compareNumbersWhenBlock,
                                        { { "__SOURCE_NAME__", "item" },
                                          { "__SOURCE_TYPE__", "item" },
                                          { "__SOURCE_VALUE__", "\"" + itemId2 + "\"" },
                                          { "__COMPARATOR__", ">=" },
                                          { "__TYPE__", "int" },
                                          { "__VALUE__", "20" } } );

    Json globalRestrictionBlock = Json::parse( block );
    globalRestrictionBlock[ "blockType" ] = "globalRestriction";

    auto thenBlock1 = replaceTokensInString(
        setItemValueThenBlock, { { "__ITEM_ID__", itemId3 }, { "__TYPE__", "int" }, { "__VALUE__", "40" } } );
    auto thenBlock2 = replaceTokensInString(
        setItemValueThenBlock, { { "__ITEM_ID__", itemId4 }, { "__TYPE__", "int" }, { "__VALUE__", "50" } } );
    auto thenBlock3 = replaceTokensInString(
        setItemValueThenBlock, { { "__ITEM_ID__", itemId5 }, { "__TYPE__", "int" }, { "__VALUE__", "60" } } );

    auto request =
        replaceTokens( createSceneWithExceptionsRequest,
                       { { "__NAME__", "compareNumbers" },
                         { "__WHEN_BLOCKS__", whenBlock },
                         { "__THEN_BLOCKS__", std::move( thenBlock1 + "," + thenBlock2 + "," + thenBlock3 ) },
                         { "__EXCEPTION_BLOCKS__", globalRestrictionBlock.dump() } } );

    SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    setItemValue( coreRemoteInterface, itemId1, 5, __LINE__ );
    setItemValue( coreRemoteInterface, itemId1, 15, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
    setItemValue( coreRemoteInterface, itemId1, 5, __LINE__ );

    setItemValue( coreRemoteInterface, itemId2, 25, __LINE__ );
    setItemValue( coreRemoteInterface, itemId1, 15, __LINE__ );
    checkBroadcasts(
        { [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, sceneId, "started" ); },
          [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, sceneId, "finished" ); },
          [this, &itemId3]( const Json& broadcast ) { return isItemUpdateBroadcast( broadcast, itemId3, 40 ); },
          [this, &itemId4]( const Json& broadcast ) { return isItemUpdateBroadcast( broadcast, itemId4, 50 ); },
          [this, &itemId5]( const Json& broadcast ) { return isItemUpdateBroadcast( broadcast, itemId5, 60 ); } },
        __LINE__ );

    deleteScene( sceneId, __LINE__ );
    removeDevice( deviceId, __LINE__ );
}

TEST_F( SceneGlobalRestrictionTests, compareStrings_exectMatch ) {  // 2.1

    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId = addDevice( coreRemoteInterface,
                               "testDevice1",
                               "smoke",
                               "134_3_24",
                               "security_sensor",
                               "smoke",
                               false /*batteryPowered*/,
                               __LINE__ );

    auto itemId1 = addItem(
        coreRemoteInterface, deviceId, "testItem1", CoreD::Model::ValueType::Enum::String, true, true, __LINE__ );
    auto itemId2 = addItem(
        coreRemoteInterface, deviceId, "testItem2", CoreD::Model::ValueType::Enum::String, true, true, __LINE__ );
    auto itemId3 =
        addItem( coreRemoteInterface, deviceId, "testItem3", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );

    auto whenBlock = replaceTokensInString( compareStringsWhenBlock,
                                            { { "__SOURCE_NAME__", "item" },
                                              { "__SOURCE_TYPE__", "item" },
                                              { "__SOURCE_VALUE__", "\"" + itemId1 + "\"" },
                                              { "__COMPARATOR__", "==" },
                                              { "__TYPE__", "string" },
                                              { "__VALUE__", "match1" } } );

    auto block = replaceTokensInString( compareStringsWhenBlock,
                                        { { "__SOURCE_NAME__", "item" },
                                          { "__SOURCE_TYPE__", "item" },
                                          { "__SOURCE_VALUE__", "\"" + itemId2 + "\"" },
                                          { "__COMPARATOR__", "==" },
                                          { "__TYPE__", "string" },
                                          { "__VALUE__", "globalRestriction" } } );
    Json globalRestrictionBlock = Json::parse( block );
    globalRestrictionBlock[ "blockType" ] = "globalRestriction";

    auto thenBlock = replaceTokensInString(
        setItemValueThenBlock, { { "__ITEM_ID__", itemId3 }, { "__TYPE__", "int" }, { "__VALUE__", "40" } } );
    auto request = replaceTokens( createSceneWithExceptionsRequest,
                                  { { "__NAME__", "compareStrings" },
                                    { "__WHEN_BLOCKS__", whenBlock },
                                    { "__THEN_BLOCKS__", thenBlock },
                                    { "__EXCEPTION_BLOCKS__", globalRestrictionBlock.dump() } } );

    SceneID sceneId = createScene( request, __LINE__ );

    setItemValue( coreRemoteInterface, itemId1, "match1", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
    setItemValue( coreRemoteInterface, itemId1, "test", __LINE__ );

    setItemValue( coreRemoteInterface, itemId2, "globalRestriction", __LINE__ );
    setItemValue( coreRemoteInterface, itemId1, "match1", __LINE__ );
    checkBroadcasts(
        { [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, sceneId, "started" ); },
          [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, sceneId, "finished" ); },
          [this, &itemId3]( const Json& broadcast ) { return isItemUpdateBroadcast( broadcast, itemId3, 40 ); } },
        __LINE__ );

    deleteScene( sceneId, __LINE__ );
    removeDevice( deviceId, __LINE__ );
}

TEST_F( SceneGlobalRestrictionTests, isItemState ) {  // 3.1

    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId = addDevice( coreRemoteInterface,
                               "testDevice1",
                               "smoke",
                               "134_3_24",
                               "security_sensor",
                               "smoke",
                               false /*batteryPowered*/,
                               __LINE__ );

    auto itemId1 = addItem(
        coreRemoteInterface, deviceId, "testItem1", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    auto itemId2 = addItem(
        coreRemoteInterface, deviceId, "testItem2", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    auto itemId3 =
        addItem( coreRemoteInterface, deviceId, "testItem3", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );

    auto whenBlock = replaceTokensInString( isItemStateWithSourceWhenBlock,
                                            { { "__SOURCE_NAME__", "item" },
                                              { "__SOURCE_TYPE__", "item" },
                                              { "__SOURCE_VALUE__", "\"" + itemId1 + "\"" },
                                              { "__TYPE__", "bool" },
                                              { "__VALUE__", "true" } } );
    auto block = replaceTokensInString( isItemStateWithSourceWhenBlock,
                                        { { "__SOURCE_NAME__", "item" },
                                          { "__SOURCE_TYPE__", "item" },
                                          { "__SOURCE_VALUE__", "\"" + itemId2 + "\"" },
                                          { "__TYPE__", "bool" },
                                          { "__VALUE__", "true" } } );

    Json globalRestrictionBlock = Json::parse( block );
    globalRestrictionBlock[ "blockType" ] = "globalRestriction";

    auto thenBlock = replaceTokensInString(
        setItemValueThenBlock, { { "__ITEM_ID__", itemId3 }, { "__TYPE__", "int" }, { "__VALUE__", "40" } } );
    auto request = replaceTokens( createSceneWithExceptionsRequest,
                                  { { "__NAME__", "isItemState" },
                                    { "__WHEN_BLOCKS__", whenBlock },
                                    { "__THEN_BLOCKS__", thenBlock },
                                    { "__EXCEPTION_BLOCKS__", globalRestrictionBlock.dump() } } );

    SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    setItemValue( coreRemoteInterface, itemId1, true, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
    setItemValue( coreRemoteInterface, itemId1, false, __LINE__ );
    setItemValue( coreRemoteInterface, itemId2, true, __LINE__ );
    setItemValue( coreRemoteInterface, itemId1, true, __LINE__ );
    checkBroadcasts(
        { [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, sceneId, "started" ); },
          [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, sceneId, "finished" ); },
          [this, &itemId3]( const Json& broadcast ) { return isItemUpdateBroadcast( broadcast, itemId3, 40 ); } },
        __LINE__ );

    deleteScene( sceneId, __LINE__ );
    removeDevice( deviceId, __LINE__ );
}

TEST_F( SceneGlobalRestrictionTests, stringOperation_contain ) {  // 4.1
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId = addDevice( coreRemoteInterface,
                               "testDevice1",
                               "smoke",
                               "134_3_24",
                               "security_sensor",
                               "smoke",
                               false /*batteryPowered*/,
                               __LINE__ );

    auto itemId1 = addItem(
        coreRemoteInterface, deviceId, "testItem1", CoreD::Model::ValueType::Enum::String, true, true, __LINE__ );
    auto itemId2 = addItem(
        coreRemoteInterface, deviceId, "testItem2", CoreD::Model::ValueType::Enum::String, true, true, __LINE__ );
    auto itemId3 =
        addItem( coreRemoteInterface, deviceId, "testItem3", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );

    auto whenBlock = replaceTokensInString( itemOperationWhenBlock,
                                            { { "__METHOD__", "stringOperation" },
                                              { "__SOURCE_NAME__", "item" },
                                              { "__SOURCE_TYPE__", "item" },
                                              { "__SOURCE_VALUE__", "\"" + itemId1 + "\"" },
                                              { "__OPERATION__", "contain" },
                                              { "__TYPE__", "string" },
                                              { "__VALUE__", R"("testing")" } } );

    auto block = replaceTokensInString( itemOperationWhenBlock,
                                        { { "__METHOD__", "stringOperation" },
                                          { "__SOURCE_NAME__", "item" },
                                          { "__SOURCE_TYPE__", "item" },
                                          { "__SOURCE_VALUE__", "\"" + itemId2 + "\"" },
                                          { "__OPERATION__", "contain" },
                                          { "__TYPE__", "string" },
                                          { "__VALUE__", R"("globalRestriction")" } } );

    Json globalRestrictionBlock = Json::parse( block );
    globalRestrictionBlock[ "blockType" ] = "globalRestriction";

    auto thenBlock = replaceTokensInString(
        setItemValueThenBlock, { { "__ITEM_ID__", itemId3 }, { "__TYPE__", "int" }, { "__VALUE__", "40" } } );
    auto request = replaceTokens( createSceneWithExceptionsRequest,
                                  { { "__NAME__", "stringOperation" },
                                    { "__WHEN_BLOCKS__", whenBlock },
                                    { "__THEN_BLOCKS__", thenBlock },
                                    { "__EXCEPTION_BLOCKS__", globalRestrictionBlock.dump() } } );

    SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    setItemValue( coreRemoteInterface, itemId1, "testing1", __LINE__ );

    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
    setItemValue( coreRemoteInterface, itemId1, "test", __LINE__ );
    setItemValue( coreRemoteInterface, itemId2, "globalRestriction", __LINE__ );
    setItemValue( coreRemoteInterface, itemId1, "testing1", __LINE__ );
    checkBroadcasts(
        { [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, sceneId, "started" ); },
          [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, sceneId, "finished" ); },
          [this, &itemId3]( const Json& broadcast ) { return isItemUpdateBroadcast( broadcast, itemId3, 40 ); } },
        __LINE__ );

    deleteScene( sceneId, __LINE__ );
    removeDevice( deviceId, __LINE__ );
}

TEST_F( SceneGlobalRestrictionTests, inArray_in ) {  // 5.1

    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId = addDevice( coreRemoteInterface,
                               "testDevice1",
                               "smoke",
                               "134_3_24",
                               "security_sensor",
                               "smoke",
                               false /*batteryPowered*/,
                               __LINE__ );

    auto itemId1 =
        addItem( coreRemoteInterface, deviceId, "testItem1", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );
    auto itemId2 =
        addItem( coreRemoteInterface, deviceId, "testItem2", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );

    auto itemId3 =
        addItem( coreRemoteInterface, deviceId, "testItem3", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );
    auto whenBlock = replaceTokensInString( itemOperationWhenBlock,
                                            { { "__METHOD__", "inArray" },
                                              { "__SOURCE_NAME__", "item" },
                                              { "__SOURCE_TYPE__", "item" },
                                              { "__SOURCE_VALUE__", "\"" + itemId1 + "\"" },
                                              { "__OPERATION__", "in" },
                                              { "__TYPE__", "array.int" },
                                              { "__VALUE__", "[1, 2, 3]" } } );

    auto block = replaceTokensInString( itemOperationWhenBlock,
                                        { { "__METHOD__", "inArray" },
                                          { "__SOURCE_NAME__", "item" },
                                          { "__SOURCE_TYPE__", "item" },
                                          { "__SOURCE_VALUE__", "\"" + itemId2 + "\"" },
                                          { "__OPERATION__", "in" },
                                          { "__TYPE__", "array.int" },
                                          { "__VALUE__", "[1, 2, 3]" } } );

    Json globalRestrictionBlock = Json::parse( block );
    globalRestrictionBlock[ "blockType" ] = "globalRestriction";

    auto thenBlock = replaceTokensInString(
        setItemValueThenBlock, { { "__ITEM_ID__", itemId3 }, { "__TYPE__", "int" }, { "__VALUE__", "40" } } );

    auto request = replaceTokens( createSceneWithExceptionsRequest,
                                  { { "__NAME__", "In_Array" },
                                    { "__WHEN_BLOCKS__", whenBlock },
                                    { "__THEN_BLOCKS__", thenBlock },
                                    { "__EXCEPTION_BLOCKS__", globalRestrictionBlock.dump() } } );

    SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    setItemValue( coreRemoteInterface, itemId1, 1, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
    setItemValue( coreRemoteInterface, itemId1, 6, __LINE__ );
    setItemValue( coreRemoteInterface, itemId2, 3, __LINE__ );
    setItemValue( coreRemoteInterface, itemId1, 2, __LINE__ );
    checkBroadcasts(
        { [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, sceneId, "started" ); },
          [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, sceneId, "finished" ); },
          [this, &itemId3]( const Json& broadcast ) { return isItemUpdateBroadcast( broadcast, itemId3, 40 ); } },
        __LINE__ );

    deleteScene( sceneId, __LINE__ );
    removeDevice( deviceId, __LINE__ );
}

TEST_F( SceneGlobalRestrictionTests, compareNumberRange ) {  // 6.1

    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId = addDevice( coreRemoteInterface,
                               "testDevice1",
                               "smoke",
                               "134_3_24",
                               "security_sensor",
                               "smoke",
                               false /*batteryPowered*/,
                               __LINE__ );

    auto itemId1 =
        addItem( coreRemoteInterface, deviceId, "testItem1", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );
    auto itemId2 =
        addItem( coreRemoteInterface, deviceId, "testItem2", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );

    auto itemId3 =
        addItem( coreRemoteInterface, deviceId, "testItem3", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );
    auto whenBlock = replaceTokensInString( compareNumbersRangeConditionBlock,
                                            { { "__SOURCE_NAME__", "item" },
                                              { "__SOURCE_TYPE__", "item" },
                                              { "__SOURCE_VALUE__", "\"" + itemId1 + "\"" },
                                              { "__COMP__", "between" } } );

    auto block = replaceTokensInString( compareNumbersRangeConditionBlock,
                                        { { "__SOURCE_NAME__", "item" },
                                          { "__SOURCE_TYPE__", "item" },
                                          { "__SOURCE_VALUE__", "\"" + itemId2 + "\"" },
                                          { "__COMP__", "between" } } );

    Json globalRestrictionBlock = Json::parse( block );
    globalRestrictionBlock[ "blockType" ] = "globalRestriction";

    auto thenBlock = replaceTokensInString(
        setItemValueThenBlock, { { "__ITEM_ID__", itemId3 }, { "__TYPE__", "int" }, { "__VALUE__", "40" } } );
    auto request = replaceTokens( createSceneWithExceptionsRequest,
                                  { { "__NAME__", "In_Array" },
                                    { "__WHEN_BLOCKS__", whenBlock },
                                    { "__THEN_BLOCKS__", thenBlock },
                                    { "__EXCEPTION_BLOCKS__", globalRestrictionBlock.dump() } } );

    SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    setItemValue( coreRemoteInterface, itemId1, 10, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    setItemValue( coreRemoteInterface, itemId2, 10, __LINE__ );
    setItemValue( coreRemoteInterface, itemId1, 20, __LINE__ );
    checkBroadcasts(
        { [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, sceneId, "started" ); },
          [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, sceneId, "finished" ); },
          [this, &itemId3]( const Json& broadcast ) { return isItemUpdateBroadcast( broadcast, itemId3, 40 ); } },
        __LINE__ );

    deleteScene( sceneId, __LINE__ );
    removeDevice( deviceId, __LINE__ );
}

TEST_F( SceneGlobalRestrictionTests, sceneCreateError ) {  // 7.1

    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId = addDevice( coreRemoteInterface,
                               "testDevice1",
                               "smoke",
                               "134_3_24",
                               "security_sensor",
                               "smoke",
                               false /*batteryPowered*/,
                               __LINE__ );

    auto itemId1 =
        addItem( coreRemoteInterface, deviceId, "testItem1", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );
    auto itemId2 =
        addItem( coreRemoteInterface, deviceId, "testItem2", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );

    auto itemId3 =
        addItem( coreRemoteInterface, deviceId, "testItem3", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );
    auto whenBlock = replaceTokensInString( compareNumbersRangeConditionBlock,
                                            { { "__SOURCE_NAME__", "item" },
                                              { "__SOURCE_TYPE__", "item" },
                                              { "__SOURCE_VALUE__", "\"" + itemId1 + "\"" },
                                              { "__COMP__", "between" } } );

    auto block = replaceTokensInString( compareNumbersRangeConditionBlock,
                                        { { "__SOURCE_NAME__", "item" },
                                          { "__SOURCE_TYPE__", "item" },
                                          { "__SOURCE_VALUE__", "\"" + itemId2 + "\"" },
                                          { "__COMP__", "between" } } );

    Json globalRestrictionBlock = Json::parse( block );
    globalRestrictionBlock[ "blockType" ] = "globalRestriction";
    globalRestrictionBlock[ "_id" ] = "test_id_123";

    auto thenBlock = replaceTokensInString(
        setItemValueThenBlock, { { "__ITEM_ID__", itemId3 }, { "__TYPE__", "int" }, { "__VALUE__", "40" } } );
    Json thenBlockJson = Json::parse( thenBlock );
    thenBlockJson[ "control" ] = JSON( "exception" << JSON( "type"
                                                            << "block_reference"
                                                            << "blockId"
                                                            << "test_id_123" ) );
    auto request = replaceTokens( createSceneWithExceptionsRequest,
                                  { { "__NAME__", "In_Array" },
                                    { "__WHEN_BLOCKS__", whenBlock },
                                    { "__THEN_BLOCKS__", thenBlockJson.dump() },
                                    { "__EXCEPTION_BLOCKS__", globalRestrictionBlock.dump() } } );

    auto broadcast = nma().sendtoHubAndGetAnswer( request );
    JsonObject error;
    json_get_obj( broadcast, "error", error );
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.wrong_field.blockId" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong field of object" );
    EXPECT_EQ( json_get_str( error, "reason" ),
               "Should not be allowed to reference globalRestrictions block in any action" );

    removeDevice( deviceId, __LINE__ );
}

TEST_F( SceneGlobalRestrictionTests, sceneStatus ) {  // 8.1

    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId = addDevice( coreRemoteInterface,
                               "testDevice1",
                               "smoke",
                               "134_3_24",
                               "security_sensor",
                               "smoke",
                               false /*batteryPowered*/,
                               __LINE__ );

    auto itemId1 =
        addItem( coreRemoteInterface, deviceId, "testItem1", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );
    auto itemId2 =
        addItem( coreRemoteInterface, deviceId, "testItem2", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );

    auto itemId3 =
        addItem( coreRemoteInterface, deviceId, "testItem3", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );
    auto whenBlock = replaceTokensInString( compareNumbersRangeConditionBlock,
                                            { { "__SOURCE_NAME__", "item" },
                                              { "__SOURCE_TYPE__", "item" },
                                              { "__SOURCE_VALUE__", "\"" + itemId1 + "\"" },
                                              { "__COMP__", "between" } } );

    auto block = replaceTokensInString( compareNumbersRangeConditionBlock,
                                        { { "__SOURCE_NAME__", "item" },
                                          { "__SOURCE_TYPE__", "item" },
                                          { "__SOURCE_VALUE__", "\"" + itemId2 + "\"" },
                                          { "__COMP__", "between" } } );

    Json globalRestrictionBlock = Json::parse( block );
    globalRestrictionBlock[ "blockType" ] = "globalRestriction";

    auto thenBlock = replaceTokensInString(
        setItemValueThenBlock, { { "__ITEM_ID__", itemId3 }, { "__TYPE__", "int" }, { "__VALUE__", "40" } } );

    auto request = replaceTokens( createSceneWithExceptionsRequest,
                                  { { "__NAME__", "In_Array" },
                                    { "__WHEN_BLOCKS__", whenBlock },
                                    { "__THEN_BLOCKS__", thenBlock },
                                    { "__EXCEPTION_BLOCKS__", globalRestrictionBlock.dump() } } );

    SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    {
        auto answer = getSceneStatus( sceneId, __LINE__ );
        JsonObject result, status;
        JsonArray globalRestriction;
        json_get_obj( answer, "result", result );
        json_get_obj( result, "status", status );
        ASSERT_TRUE( json_contains( status, "globalRestriction" ) );
        json_get_arr( status, "globalRestriction", globalRestriction );
        ASSERT_EQ( json_get_str( globalRestriction[ 0 ], "state" ), "false" );
    }

    setItemValue( coreRemoteInterface, itemId2, 10, __LINE__ );

    {
        auto answer = getSceneStatus( sceneId, __LINE__ );
        JsonObject result, status;
        JsonArray globalRestriction;
        json_get_obj( answer, "result", result );
        json_get_obj( result, "status", status );
        ASSERT_TRUE( json_contains( status, "globalRestriction" ) );
        json_get_arr( status, "globalRestriction", globalRestriction );
        ASSERT_EQ( json_get_str( globalRestriction[ 0 ], "state" ), "true" );
    }

    deleteScene( sceneId, __LINE__ );
    removeDevice( deviceId, __LINE__ );
}

TEST_F( SceneGlobalRestrictionTests, AndOperator ) {  // 9.1

    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId = addDevice( coreRemoteInterface,
                               "testDevice1",
                               "smoke",
                               "134_3_24",
                               "security_sensor",
                               "smoke",
                               false /*batteryPowered*/,
                               __LINE__ );

    auto itemId1 = addItem(
        coreRemoteInterface, deviceId, "testItem1", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    auto itemId2 = addItem(
        coreRemoteInterface, deviceId, "testItem2", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    auto itemId3 = addItem(
        coreRemoteInterface, deviceId, "testItem3", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    auto itemId4 =
        addItem( coreRemoteInterface, deviceId, "testItem4", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );

    auto whenBlock = replaceTokensInString( isItemStateWithSourceWhenBlock,
                                            { { "__SOURCE_NAME__", "item" },
                                              { "__SOURCE_TYPE__", "item" },
                                              { "__SOURCE_VALUE__", "\"" + itemId1 + "\"" },
                                              { "__TYPE__", "bool" },
                                              { "__VALUE__", "true" } } );
    auto block1 = replaceTokensInString( isItemStateWithSourceWhenBlock,
                                         { { "__SOURCE_NAME__", "item" },
                                           { "__SOURCE_TYPE__", "item" },
                                           { "__SOURCE_VALUE__", "\"" + itemId2 + "\"" },
                                           { "__TYPE__", "bool" },
                                           { "__VALUE__", "true" } } );

    auto block2 = replaceTokensInString( isItemStateWithSourceWhenBlock,
                                         { { "__SOURCE_NAME__", "item" },
                                           { "__SOURCE_TYPE__", "item" },
                                           { "__SOURCE_VALUE__", "\"" + itemId3 + "\"" },
                                           { "__TYPE__", "bool" },
                                           { "__VALUE__", "true" } } );

    block1 += ",";
    block1.append( std::move( block2 ) );
    auto andOperator = replaceTokensInString( andOperatorWhenBlock, { { "__BLOCKS__", block1 } } );
    Json globalRestrictionBlock = Json::parse( andOperator );
    globalRestrictionBlock[ "blockType" ] = "globalRestriction";

    auto thenBlock = replaceTokensInString(
        setItemValueThenBlock, { { "__ITEM_ID__", itemId4 }, { "__TYPE__", "int" }, { "__VALUE__", "40" } } );
    auto request = replaceTokens( createSceneWithExceptionsRequest,
                                  { { "__NAME__", "isItemState" },
                                    { "__WHEN_BLOCKS__", whenBlock },
                                    { "__THEN_BLOCKS__", thenBlock },
                                    { "__EXCEPTION_BLOCKS__", globalRestrictionBlock.dump() } } );

    SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    setItemValue( coreRemoteInterface, itemId2, true, __LINE__ );
    setItemValue( coreRemoteInterface, itemId1, true, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
    setItemValue( coreRemoteInterface, itemId1, false, __LINE__ );
    setItemValue( coreRemoteInterface, itemId3, true, __LINE__ );
    setItemValue( coreRemoteInterface, itemId1, true, __LINE__ );
    checkBroadcasts(
        { [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, sceneId, "started" ); },
          [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, sceneId, "finished" ); },
          [this, &itemId4]( const Json& broadcast ) { return isItemUpdateBroadcast( broadcast, itemId4, 40 ); } },
        __LINE__ );

    deleteScene( sceneId, __LINE__ );
    removeDevice( deviceId, __LINE__ );
}

TEST_F( SceneGlobalRestrictionTests, OrOperator ) {  // 10.1

    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId = addDevice( coreRemoteInterface,
                               "testDevice1",
                               "smoke",
                               "134_3_24",
                               "security_sensor",
                               "smoke",
                               false /*batteryPowered*/,
                               __LINE__ );

    auto itemId1 = addItem(
        coreRemoteInterface, deviceId, "testItem1", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    auto itemId2 = addItem(
        coreRemoteInterface, deviceId, "testItem2", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    auto itemId3 = addItem(
        coreRemoteInterface, deviceId, "testItem3", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    auto itemId4 =
        addItem( coreRemoteInterface, deviceId, "testItem4", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );

    auto whenBlock = replaceTokensInString( isItemStateWithSourceWhenBlock,
                                            { { "__SOURCE_NAME__", "item" },
                                              { "__SOURCE_TYPE__", "item" },
                                              { "__SOURCE_VALUE__", "\"" + itemId1 + "\"" },
                                              { "__TYPE__", "bool" },
                                              { "__VALUE__", "true" } } );
    auto block1 = replaceTokensInString( isItemStateWithSourceWhenBlock,
                                         { { "__SOURCE_NAME__", "item" },
                                           { "__SOURCE_TYPE__", "item" },
                                           { "__SOURCE_VALUE__", "\"" + itemId2 + "\"" },
                                           { "__TYPE__", "bool" },
                                           { "__VALUE__", "true" } } );

    auto block2 = replaceTokensInString( isItemStateWithSourceWhenBlock,
                                         { { "__SOURCE_NAME__", "item" },
                                           { "__SOURCE_TYPE__", "item" },
                                           { "__SOURCE_VALUE__", "\"" + itemId3 + "\"" },
                                           { "__TYPE__", "bool" },
                                           { "__VALUE__", "true" } } );

    block1 += ",";
    block1.append( std::move( block2 ) );
    auto orOperator = replaceTokensInString( orOperatorWhenBlock, { { "__BLOCKS__", block1 } } );
    Json globalRestrictionBlock = Json::parse( orOperator );
    globalRestrictionBlock[ "blockType" ] = "globalRestriction";

    auto thenBlock = replaceTokensInString(
        setItemValueThenBlock, { { "__ITEM_ID__", itemId4 }, { "__TYPE__", "int" }, { "__VALUE__", "40" } } );
    auto request = replaceTokens( createSceneWithExceptionsRequest,
                                  { { "__NAME__", "isItemState" },
                                    { "__WHEN_BLOCKS__", whenBlock },
                                    { "__THEN_BLOCKS__", thenBlock },
                                    { "__EXCEPTION_BLOCKS__", globalRestrictionBlock.dump() } } );

    SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    setItemValue( coreRemoteInterface, itemId2, true, __LINE__ );
    setItemValue( coreRemoteInterface, itemId1, true, __LINE__ );
    checkBroadcasts(
        { [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, sceneId, "started" ); },
          [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, sceneId, "finished" ); },
          [this, &itemId4]( const Json& broadcast ) { return isItemUpdateBroadcast( broadcast, itemId4, 40 ); } },
        __LINE__ );
    setItemValue( coreRemoteInterface, itemId1, false, __LINE__ );
    setItemValue( coreRemoteInterface, itemId2, false, __LINE__ );
    setItemValue( coreRemoteInterface, itemId4, 30, __LINE__ );
    setItemValue( coreRemoteInterface, itemId3, true, __LINE__ );
    setItemValue( coreRemoteInterface, itemId1, true, __LINE__ );
    checkBroadcasts(
        { [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, sceneId, "started" ); },
          [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, sceneId, "finished" ); },
          [this, &itemId4]( const Json& broadcast ) { return isItemUpdateBroadcast( broadcast, itemId4, 40 ); } },
        __LINE__ );

    deleteScene( sceneId, __LINE__ );
    removeDevice( deviceId, __LINE__ );
}

TEST_F( SceneGlobalRestrictionTests, notOperator ) {  // 11.1

    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId = addDevice( coreRemoteInterface,
                               "testDevice1",
                               "smoke",
                               "134_3_24",
                               "security_sensor",
                               "smoke",
                               false /*batteryPowered*/,
                               __LINE__ );

    auto itemId1 = addItem(
        coreRemoteInterface, deviceId, "testItem1", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    auto itemId2 = addItem(
        coreRemoteInterface, deviceId, "testItem2", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    auto itemId3 =
        addItem( coreRemoteInterface, deviceId, "testItem3", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );

    auto whenBlock = replaceTokensInString( isItemStateWithSourceWhenBlock,
                                            { { "__SOURCE_NAME__", "item" },
                                              { "__SOURCE_TYPE__", "item" },
                                              { "__SOURCE_VALUE__", "\"" + itemId1 + "\"" },
                                              { "__TYPE__", "bool" },
                                              { "__VALUE__", "true" } } );
    auto block = replaceTokensInString( isItemStateWithSourceWhenBlock,
                                        { { "__SOURCE_NAME__", "item" },
                                          { "__SOURCE_TYPE__", "item" },
                                          { "__SOURCE_VALUE__", "\"" + itemId2 + "\"" },
                                          { "__TYPE__", "bool" },
                                          { "__VALUE__", "true" } } );

    auto notOperator = replaceTokensInString( notOperatorWhenBlock, { { "__BLOCK__", block } } );
    Json globalRestrictionBlock = Json::parse( notOperator );
    globalRestrictionBlock[ "blockType" ] = "globalRestriction";

    auto thenBlock = replaceTokensInString(
        setItemValueThenBlock, { { "__ITEM_ID__", itemId3 }, { "__TYPE__", "int" }, { "__VALUE__", "40" } } );
    auto request = replaceTokens( createSceneWithExceptionsRequest,
                                  { { "__NAME__", "isItemState" },
                                    { "__WHEN_BLOCKS__", whenBlock },
                                    { "__THEN_BLOCKS__", thenBlock },
                                    { "__EXCEPTION_BLOCKS__", globalRestrictionBlock.dump() } } );

    SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    setItemValue( coreRemoteInterface, itemId2, true, __LINE__ );
    setItemValue( coreRemoteInterface, itemId1, true, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
    setItemValue( coreRemoteInterface, itemId1, false, __LINE__ );
    setItemValue( coreRemoteInterface, itemId2, false, __LINE__ );
    setItemValue( coreRemoteInterface, itemId1, true, __LINE__ );

    checkBroadcasts(
        { [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, sceneId, "started" ); },
          [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, sceneId, "finished" ); },
          [this, &itemId3]( const Json& broadcast ) { return isItemUpdateBroadcast( broadcast, itemId3, 40 ); } },
        __LINE__ );

    deleteScene( sceneId, __LINE__ );
    removeDevice( deviceId, __LINE__ );
}

TEST_F( SceneGlobalRestrictionTests, initialStateOfGlobalRestriction ) {  // 12.1

    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId = addDevice( coreRemoteInterface,
                               "testDevice1",
                               "smoke",
                               "134_3_24",
                               "security_sensor",
                               "smoke",
                               false /*batteryPowered*/,
                               __LINE__ );

    auto itemId1 =
        addItem( coreRemoteInterface, deviceId, "testItem1", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );
    auto itemId2 =
        addItem( coreRemoteInterface, deviceId, "testItem2", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );

    auto itemId3 =
        addItem( coreRemoteInterface, deviceId, "testItem3", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );

    setItemValue( coreRemoteInterface, itemId2, 10, __LINE__ );

    auto whenBlock = replaceTokensInString( compareNumbersRangeConditionBlock,
                                            { { "__SOURCE_NAME__", "item" },
                                              { "__SOURCE_TYPE__", "item" },
                                              { "__SOURCE_VALUE__", "\"" + itemId1 + "\"" },
                                              { "__COMP__", "between" } } );

    auto block = replaceTokensInString( compareNumbersRangeConditionBlock,
                                        { { "__SOURCE_NAME__", "item" },
                                          { "__SOURCE_TYPE__", "item" },
                                          { "__SOURCE_VALUE__", "\"" + itemId2 + "\"" },
                                          { "__COMP__", "between" } } );

    Json globalRestrictionBlock = Json::parse( block );
    globalRestrictionBlock[ "blockType" ] = "globalRestriction";

    auto thenBlock = replaceTokensInString(
        setItemValueThenBlock, { { "__ITEM_ID__", itemId3 }, { "__TYPE__", "int" }, { "__VALUE__", "40" } } );

    auto request = replaceTokens( createSceneWithExceptionsRequest,
                                  { { "__NAME__", "In_Array" },
                                    { "__WHEN_BLOCKS__", whenBlock },
                                    { "__THEN_BLOCKS__", thenBlock },
                                    { "__EXCEPTION_BLOCKS__", globalRestrictionBlock.dump() } } );

    SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    {
        auto answer = getSceneStatus( sceneId, __LINE__ );
        JsonObject result, status;
        JsonArray globalRestriction;
        json_get_obj( answer, "result", result );
        json_get_obj( result, "status", status );
        ASSERT_TRUE( json_contains( status, "globalRestriction" ) );
        json_get_arr( status, "globalRestriction", globalRestriction );
        ASSERT_EQ( json_get_str( globalRestriction[ 0 ], "state" ), "true" );
    }

    setItemValue( coreRemoteInterface, itemId1, 10, __LINE__ );

    checkBroadcasts(
        { [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, sceneId, "started" ); },
          [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, sceneId, "finished" ); },
          [this, &itemId3]( const Json& broadcast ) { return isItemUpdateBroadcast( broadcast, itemId3, 40 ); } },
        __LINE__ );

    deleteScene( sceneId, __LINE__ );
    removeDevice( deviceId, __LINE__ );
}

TEST_F( SceneGlobalRestrictionTests, AndOperatorInitialState ) {  // 13.1

    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId = addDevice( coreRemoteInterface,
                               "testDevice1",
                               "smoke",
                               "134_3_24",
                               "security_sensor",
                               "smoke",
                               false /*batteryPowered*/,
                               __LINE__ );

    auto itemId1 = addItem(
        coreRemoteInterface, deviceId, "testItem1", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    auto itemId2 = addItem(
        coreRemoteInterface, deviceId, "testItem2", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    auto itemId3 = addItem(
        coreRemoteInterface, deviceId, "testItem3", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    auto itemId4 =
        addItem( coreRemoteInterface, deviceId, "testItem4", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );

    setItemValue( coreRemoteInterface, itemId2, true, __LINE__ );
    setItemValue( coreRemoteInterface, itemId3, true, __LINE__ );

    auto whenBlock = replaceTokensInString( isItemStateWithSourceWhenBlock,
                                            { { "__SOURCE_NAME__", "item" },
                                              { "__SOURCE_TYPE__", "item" },
                                              { "__SOURCE_VALUE__", "\"" + itemId1 + "\"" },
                                              { "__TYPE__", "bool" },
                                              { "__VALUE__", "true" } } );
    auto block1 = replaceTokensInString( isItemStateWithSourceWhenBlock,
                                         { { "__SOURCE_NAME__", "item" },
                                           { "__SOURCE_TYPE__", "item" },
                                           { "__SOURCE_VALUE__", "\"" + itemId2 + "\"" },
                                           { "__TYPE__", "bool" },
                                           { "__VALUE__", "true" } } );

    auto block2 = replaceTokensInString( isItemStateWithSourceWhenBlock,
                                         { { "__SOURCE_NAME__", "item" },
                                           { "__SOURCE_TYPE__", "item" },
                                           { "__SOURCE_VALUE__", "\"" + itemId3 + "\"" },
                                           { "__TYPE__", "bool" },
                                           { "__VALUE__", "true" } } );

    block1 += ",";
    block1.append( std::move( block2 ) );
    auto andOperator = replaceTokensInString( andOperatorWhenBlock, { { "__BLOCKS__", block1 } } );
    Json globalRestrictionBlock = Json::parse( andOperator );
    globalRestrictionBlock[ "blockType" ] = "globalRestriction";

    auto thenBlock = replaceTokensInString(
        setItemValueThenBlock, { { "__ITEM_ID__", itemId4 }, { "__TYPE__", "int" }, { "__VALUE__", "40" } } );
    auto request = replaceTokens( createSceneWithExceptionsRequest,
                                  { { "__NAME__", "isItemState" },
                                    { "__WHEN_BLOCKS__", whenBlock },
                                    { "__THEN_BLOCKS__", thenBlock },
                                    { "__EXCEPTION_BLOCKS__", globalRestrictionBlock.dump() } } );

    SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    {
        auto answer = getSceneStatus( sceneId, __LINE__ );
        JsonObject result, status;
        JsonArray globalRestriction;
        json_get_obj( answer, "result", result );
        json_get_obj( result, "status", status );
        ASSERT_TRUE( json_contains( status, "globalRestriction" ) );
        json_get_arr( status, "globalRestriction", globalRestriction );
        ASSERT_EQ( json_get_str( globalRestriction[ 0 ], "state" ), "true" );
    }

    setItemValue( coreRemoteInterface, itemId1, true, __LINE__ );
    checkBroadcasts(
        { [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, sceneId, "started" ); },
          [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, sceneId, "finished" ); },
          [this, &itemId4]( const Json& broadcast ) { return isItemUpdateBroadcast( broadcast, itemId4, 40 ); } },
        __LINE__ );

    deleteScene( sceneId, __LINE__ );
    removeDevice( deviceId, __LINE__ );
}

TEST_F( SceneGlobalRestrictionTests, luaSceneModuleGetState ) {  // 14.1

    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto deviceId = addDevice( coreRemoteInterface,
                               "testDevice1",
                               "smoke",
                               "134_3_24",
                               "security_sensor",
                               "smoke",
                               false /*batteryPowered*/,
                               __LINE__ );

    auto itemId1 =
        addItem( coreRemoteInterface, deviceId, "testItem1", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );
    auto itemId2 =
        addItem( coreRemoteInterface, deviceId, "testItem2", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );

    auto itemId3 =
        addItem( coreRemoteInterface, deviceId, "testItem3", CoreD::Model::ValueType::Enum::Int, true, true, __LINE__ );

    auto whenBlock = replaceTokensInString( compareNumbersRangeConditionBlock,
                                            { { "__SOURCE_NAME__", "item" },
                                              { "__SOURCE_TYPE__", "item" },
                                              { "__SOURCE_VALUE__", "\"" + itemId1 + "\"" },
                                              { "__COMP__", "between" } } );

    auto block = replaceTokensInString( compareNumbersRangeConditionBlock,
                                        { { "__SOURCE_NAME__", "item" },
                                          { "__SOURCE_TYPE__", "item" },
                                          { "__SOURCE_VALUE__", "\"" + itemId2 + "\"" },
                                          { "__COMP__", "between" } } );

    Json globalRestrictionBlock = Json::parse( block );
    globalRestrictionBlock[ "blockType" ] = "globalRestriction";

    auto thenBlock = replaceTokensInString(
        setItemValueThenBlock, { { "__ITEM_ID__", itemId3 }, { "__TYPE__", "int" }, { "__VALUE__", "40" } } );

    auto request = replaceTokens( createSceneWithExceptionsRequest,
                                  { { "__NAME__", "In_Array" },
                                    { "__WHEN_BLOCKS__", whenBlock },
                                    { "__THEN_BLOCKS__", thenBlock },
                                    { "__EXCEPTION_BLOCKS__", globalRestrictionBlock.dump() } } );

    Json scene;
    SceneID sceneId = createScene( request, scene, __LINE__ );
    Json blockId;
    ASSERT_TRUE( json_get( scene, { "exceptions", 0, "_id" }, blockId ) );

    std::string scriptPath = "HUB:scenes_test/scripts/get_when_block_state";
    Json params = JSON( "scene_id" << sceneId << "block_id" << json_get_str( blockId ) );

    runScriptWithoutChecks( scriptPath, params, __LINE__ );
    JsonObject broadcast = waitForBroadcast( "hub.extensions.plugin.ui_broadcast", getCodeLineString( __LINE__ ) );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );
    JsonObject result;
    ASSERT_TRUE( json_get_obj( broadcast, "result", result ) );
    ASSERT_EQ( json_get_str( result, "method" ), "compareNumberRange" );
    ASSERT_EQ( json_get_str( result, "state" ), "false" );

    setItemValue( coreRemoteInterface, itemId2, 10, __LINE__ );

    runScriptWithoutChecks( scriptPath, params, __LINE__ );
    broadcast = waitForBroadcast( "hub.extensions.plugin.ui_broadcast", getCodeLineString( __LINE__ ) );

    checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
    checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );

    ASSERT_TRUE( json_get_obj( broadcast, "result", result ) );
    ASSERT_EQ( json_get_str( result, "method" ), "compareNumberRange" );
    ASSERT_EQ( json_get_str( result, "state" ), "true" );

    deleteScene( sceneId, __LINE__ );
    removeDevice( deviceId, __LINE__ );
}

}  // namespace Tests
}  // namespace HubPlatform
