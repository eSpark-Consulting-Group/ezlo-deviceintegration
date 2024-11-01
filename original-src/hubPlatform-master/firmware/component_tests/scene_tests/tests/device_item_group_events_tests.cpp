#include "common_test_resources/fixtures/device_item_groups_fixture.h"
#include "common_test_resources/resources/scene_manager_resources.h"
#include "common_test_resources/resources/cored_resources.h"

#include <gtest/gtest.h>
#include "tiny/tiny_consolecolors.h"

namespace HubPlatform {
namespace Tests {

/*

TESTPLAN

DONE    1 CompareNumbersEvent
DONE        1.1 Create device item group event with exact type
DONE    2 CompareStringsEvent
DONE        2.1 Create device item group event with exact type
DONE    3 CompareValuesEvent
DONE        3.1 Create device item group event with exact type
DONE    4 IsItemStateEvent
DONE        4.1 Create device item group event with exact type
DONE        4.2 Create device item for 'armed' device group event with exact type
DONE    5 IsDeviceStateEvent
DONE        5.1 Create device item group event with exact type
DONE    6 StringOperationEvent
DONE        6.1 Create device item group event with exact type
DONE    7 InArrayEvent
DONE        7.1 Create device item group event with exact type
DONE    8 CompareNumberRangeEvent
DONE        8.1 Create device item group event with exact type
DONE    9 And Operator
DONE        9.1 Create device item group event with exact type
DONE    10 IsItemStateEvent
DONE        10.1 Delete triggers in edit scene to check scene runs

*/

class SceneDeviceItemGroupEventsTests : public DeviceItemGroupsFixture {
public:
    void SetUp() override;
    void TearDown() override;
};

void SceneDeviceItemGroupEventsTests::SetUp() {
    DeviceItemGroupsFixture::SetUp();
    callingFile( __FILE__ );
}

void SceneDeviceItemGroupEventsTests::TearDown() {
    clearExistingDevices( __LINE__ );
    DeviceItemGroupsFixture::TearDown();
}

TEST_F( SceneDeviceItemGroupEventsTests, compareNumbers_exactMatch ) {  // 1.1
    const std::string deviceCategory = "security_sensor";
    const std::string deviceSubcategory = "motion";

    const std::string testItemName = "testItemName";

    // create device group
    auto createGroupRequest = Json::parse(
        replaceTokensInString( scenesDeviceGroupCreate, { { "_DEVICE_GROUP_NAME_", "compareNumbers_exactHash" } } ) );
    createGroupRequest[ "params" ][ "categories" ] =
        JSON_ARRAY( JSON( "category" << deviceCategory << "subcategories" << JSON_ARRAY( deviceSubcategory ) ) );
    auto deviceGroupId = createDeviceGroup( createGroupRequest, __LINE__ );

    // create item group
    // DONE add item groups when they will be implemented
    auto createItemGroupRequest = replaceTokens(
        scenesItemGroupCreate,
        { { "__NAME__", "testItemGroup" }, { "__ITEM_NAME__", testItemName }, { "__VALUE_TYPE__", "int" } } );
    json_remove( createItemGroupRequest[ "params" ], "enum" );

    auto itemGroupId = createItemGroup( createItemGroupRequest, __LINE__ );

    // create devices matching these groups
    auto deviceId = makeDevice( deviceCategory, deviceSubcategory, __LINE__ );
    auto itemId = makeIntItem( deviceId, testItemName, 0, __LINE__ );

    // create a scene pointing to the device group
    auto block = replaceTokensInString( compareNumbersWhenBlock,
                                        { { "__SOURCE_NAME__", "deviceGroup" },
                                          { "__SOURCE_TYPE__", "device_group" },
                                          { "__SOURCE_VALUE__", quote( deviceGroupId ) },
                                          { "__COMPARATOR__", ">=" },
                                          { "__TYPE__", "int" },
                                          { "__VALUE__", "10" } } );

    // add item group to args and fields
    auto fieldItemGroupSource = replaceTokens( sceneEventFieldsSource,
                                               { { "__SOURCE_NAME__", "itemGroup" },
                                                 { "__SOURCE_TYPE__", "item_group" },
                                                 { "__SOURCE_VALUE__", quote( itemGroupId ) } } );

    Json whenBlock = Json::parse( block );
    // appendArgument
    whenBlock[ "blockOptions" ][ "method" ][ "args" ][ "itemGroup" ] = "itemGroup";
    // appendField
    whenBlock[ "fields" ].push_back( fieldItemGroupSource );

    auto request = replaceTokens( createSceneRequest,
                                  { { "__NAME__", "compareNumbersDeviceItemGroups" },
                                    { "__WHEN_BLOCKS__", whenBlock.dump() },
                                    { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    // the scene should be started by compareNumbers block
    setItemValue( m_remote, itemId, 10, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    // CompareNumberEvent should be stateless, scene should be triggered again
    setItemValue( m_remote, itemId, 20, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    deleteScene( sceneId, __LINE__ );
    deleteItemGroup( itemGroupId, __LINE__ );
    deleteDeviceGroup( deviceGroupId, __LINE__ );
}

TEST_F( SceneDeviceItemGroupEventsTests, compareStrings_exectMatch ) {  // 2.1
    const std::string deviceCategory = "security_sensor";
    const std::string deviceSubcategory = "motion";
    const std::string testItemName = "testItemName";

    auto createGroupRequest = Json::parse( scenesDeviceGroupCreate );
    createGroupRequest[ "params" ][ "name" ] = "compareStrings_exactHash";
    createGroupRequest[ "params" ][ "categories" ] =
        JSON_ARRAY( JSON( "category" << deviceCategory << "subcategories" << JSON_ARRAY( deviceSubcategory ) ) );
    auto deviceGroupId = createDeviceGroup( createGroupRequest, __LINE__ );

    // create item group
    // DONE add item groups when they will be implemented
    auto createItemGroupRequest = replaceTokens(
        scenesItemGroupCreate,
        { { "__NAME__", "testItemGroup" }, { "__ITEM_NAME__", testItemName }, { "__VALUE_TYPE__", "string" } } );
    json_remove( createItemGroupRequest[ "params" ], "enum" );

    auto itemGroupId = createItemGroup( createItemGroupRequest, __LINE__ );

    auto deviceId = makeDevice( deviceCategory, deviceSubcategory, __LINE__ );
    auto itemId = makeStringItem( deviceId, testItemName, "", __LINE__ );

    auto block = replaceTokensInString( compareStringsWhenBlock,
                                        { { "__SOURCE_NAME__", "deviceGroup" },
                                          { "__SOURCE_TYPE__", "device_group" },
                                          { "__SOURCE_VALUE__", quote( deviceGroupId ) },
                                          { "__COMPARATOR__", ">=" },
                                          { "__TYPE__", "string" },
                                          { "__VALUE__", "match0" } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "compareStringsDeviceItemGroups" }, { "__WHEN_BLOCKS__", block }, { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    // the scene should be started by compareStrings block
    setItemValue( m_remote, itemId, "match1", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    // CompareStringsEvent should be stateless, scene should be triggered again
    setItemValue( m_remote, itemId, "match2", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    deleteScene( sceneId, __LINE__ );
    deleteItemGroup( itemGroupId, __LINE__ );
    deleteDeviceGroup( deviceGroupId, __LINE__ );
}

TEST_F( SceneDeviceItemGroupEventsTests, compareValue_exectMatch ) {  // 3.1
    const std::string deviceCategory = "security_sensor";
    const std::string deviceSubcategory = "motion";
    const std::string testItemName = "testItemName";

    auto createGroupRequest = Json::parse( scenesDeviceGroupCreate );
    createGroupRequest[ "params" ][ "name" ] = "compareValue_exactHash";
    createGroupRequest[ "params" ][ "categories" ] =
        JSON_ARRAY( JSON( "category" << deviceCategory << "subcategories" << JSON_ARRAY( deviceSubcategory ) ) );
    auto deviceGroupId = createDeviceGroup( createGroupRequest, __LINE__ );

    // create item group
    // DONE add item groups when they will be implemented
    auto createItemGroupRequest = replaceTokens(
        scenesItemGroupCreate,
        { { "__NAME__", "testItemGroup" }, { "__ITEM_NAME__", testItemName }, { "__VALUE_TYPE__", "string" } } );
    json_remove( createItemGroupRequest[ "params" ], "enum" );

    auto itemGroupId = createItemGroup( createItemGroupRequest, __LINE__ );

    auto deviceId = makeDevice( deviceCategory, deviceSubcategory, __LINE__ );
    auto itemId = makeStringItem( deviceId, testItemName, "", __LINE__ );

    std::string block;
    block = replaceTokensInString( compareValuesWhenBlock,
                                   { { "__SOURCE_NAME__", "deviceGroup" },
                                     { "__SOURCE_TYPE__", "device_group" },
                                     { "__SOURCE_VALUE__", quote( deviceGroupId ) },
                                     { "__COMPARATOR__", ">=" },
                                     { "__VALUE_TYPE_VALUE__", "string" },
                                     { "__TYPE__", "string" },
                                     { "__UNQUOTED_VALUE__", R"("match0")" } } );

    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "compareValuesDeviceItemGroups" }, { "__WHEN_BLOCKS__", block }, { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    // the scene should be started by compareStrings block
    setItemValue( m_remote, itemId, "match1", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    // CompareValueEvent should be stateless, scene should be triggered again
    setItemValue( m_remote, itemId, "match2", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    deleteScene( sceneId, __LINE__ );
    deleteItemGroup( itemGroupId, __LINE__ );
    deleteDeviceGroup( deviceGroupId, __LINE__ );
}

TEST_F( SceneDeviceItemGroupEventsTests, isItemStateDeviceGroup ) {  // 4.1
    const std::string deviceCategory = "security_sensor";
    const std::string deviceSubcategory = "motion";
    const std::string testItemName = "testItemName";

    auto createGroupRequest = Json::parse( scenesDeviceGroupCreate );
    createGroupRequest[ "params" ][ "name" ] = "compareValue_exactHash";
    createGroupRequest[ "params" ][ "categories" ] =
        JSON_ARRAY( JSON( "category" << deviceCategory << "subcategories" << JSON_ARRAY( deviceSubcategory ) ) );
    auto deviceGroupId = createDeviceGroup( createGroupRequest, __LINE__ );

    // create item group
    // DONE add item groups when they will be implemented
    auto createItemGroupRequest = replaceTokens(
        scenesItemGroupCreate,
        { { "__NAME__", "testItemGroup" }, { "__ITEM_NAME__", testItemName }, { "__VALUE_TYPE__", "string" } } );
    json_remove( createItemGroupRequest[ "params" ], "enum" );

    auto itemGroupId = createItemGroup( createItemGroupRequest, __LINE__ );

    auto deviceId = makeDevice( deviceCategory, deviceSubcategory, __LINE__ );
    auto itemId = makeStringItem( deviceId, testItemName, "", __LINE__ );

    std::string block;
    block = replaceTokensInString( isItemStateWithSourceWhenBlock,
                                   { { "__SOURCE_NAME__", "deviceGroup" },
                                     { "__SOURCE_TYPE__", "device_group" },
                                     { "__SOURCE_VALUE__", quote( deviceGroupId ) },
                                     { "__TYPE__", "string" },
                                     { "__VALUE__", R"("match1")" } } );

    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "isItemStateDeviceItemGroups" }, { "__WHEN_BLOCKS__", block }, { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    // the scene should be started by compareStrings block
    setItemValue( m_remote, itemId, "match1", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    setItemValue( m_remote, itemId, "match2", __LINE__ );
    // there are no broadcasts

    setItemValue( m_remote, itemId, "match1", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    deleteScene( sceneId, __LINE__ );
    deleteItemGroup( itemGroupId, __LINE__ );
    deleteDeviceGroup( deviceGroupId, __LINE__ );
}

TEST_F( SceneDeviceItemGroupEventsTests, isItemStateDeviceWithArmedDeviceGroup ) {  // 4.2
    const std::string deviceCategory = "security_sensor";
    const std::string deviceSubcategory = "smoke";
    const std::string testItemName = "security_threat";

    // Set alarmDelay: 30 to 0
    makeRemote().setAlarmDelay( 0 );

    auto createGroupRequest = Json::parse( scenesDeviceGroupCreate );
    createGroupRequest[ "params" ][ "name" ] = "compareValue_exactHash";
    createGroupRequest[ "params" ][ "categories" ] =
        JSON_ARRAY( JSON( "category" << deviceCategory << "subcategories" << JSON_ARRAY( deviceSubcategory ) ) );
    auto deviceGroupId = createDeviceGroup( createGroupRequest, __LINE__ );
    auto createItemGroupRequest = replaceTokens(
        scenesItemGroupCreate,
        { { "__NAME__", "testItemGroup" }, { "__ITEM_NAME__", testItemName }, { "__VALUE_TYPE__", "bool" } } );
    json_remove( createItemGroupRequest[ "params" ], "enum" );

    auto itemGroupId = createItemGroup( createItemGroupRequest, __LINE__ );
    auto deviceId = makeDevice( deviceCategory, deviceSubcategory, __LINE__ );

    auto itemId =
        addItem( m_remote, deviceId, testItemName, CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    checkDeviceUpdateBroadcast( deviceId, { { "armed", true } }, __LINE__ );

    std::string block;
    block = replaceTokensInString( isItemStateWithSourceWhenBlock,
                                   { { "__SOURCE_NAME__", "deviceGroup" },
                                     { "__SOURCE_TYPE__", "device_group" },
                                     { "__SOURCE_VALUE__", quote( deviceGroupId ) },
                                     { "__TYPE__", "bool" },
                                     { "__VALUE__", "true" } } );

    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "isItemStateDeviceItemGroups" }, { "__WHEN_BLOCKS__", block }, { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    // the scene should be started by compareStrings block
    setItemValue( m_remote, itemId, true, __LINE__ );
    checkBroadcasts(
        { [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, request, sceneId, "started" ); },
          [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, request, sceneId, "finished" ); },
          []( const Json& broadcast ) { return json_get_str( broadcast, "msg_subclass" ) == "hub.modes.alarmed"; } },
        __LINE__ );

    setItemValue( m_remote, itemId, false, __LINE__ );
    //    // there are no broadcasts

    setItemValue( m_remote, itemId, true, __LINE__ );

    checkBroadcasts(
        { [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, request, sceneId, "started" ); },
          [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, request, sceneId, "finished" ); },
          []( const Json& broadcast ) { return json_get_str( broadcast, "msg_subclass" ) == "hub.modes.alarmed"; } },
        __LINE__ );

    deleteScene( sceneId, __LINE__ );
    deleteItemGroup( itemGroupId, __LINE__ );
    deleteDeviceGroup( deviceGroupId, __LINE__ );
}

TEST_F( SceneDeviceItemGroupEventsTests, isDeviceStateDeviceGroup ) {  // 5.1
    const std::string deviceCategory = "security_sensor";
    const std::string deviceSubcategory = "smoke";
    const std::string testItemName = "security_threat";

    auto createGroupRequest = Json::parse( scenesDeviceGroupCreate );
    createGroupRequest[ "params" ][ "name" ] = "compareValue_exactHash";
    createGroupRequest[ "params" ][ "categories" ] =
        JSON_ARRAY( JSON( "category" << deviceCategory << "subcategories" << JSON_ARRAY( deviceSubcategory ) ) );
    auto deviceGroupId = createDeviceGroup( createGroupRequest, __LINE__ );
    auto createItemGroupRequest = replaceTokens(
        scenesItemGroupCreate,
        { { "__NAME__", "testItemGroup" }, { "__ITEM_NAME__", testItemName }, { "__VALUE_TYPE__", "bool" } } );
    json_remove( createItemGroupRequest[ "params" ], "enum" );

    auto itemGroupId = createItemGroup( createItemGroupRequest, __LINE__ );
    auto deviceId = makeDevice( deviceCategory, deviceSubcategory, __LINE__ );

    auto itemId =
        addItem( m_remote, deviceId, testItemName, CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    checkDeviceUpdateBroadcast( deviceId, { { "armed", true } }, __LINE__ );

    auto deviceId1 = makeDevice( deviceCategory, deviceSubcategory, __LINE__ );

    auto itemId1 =
        addItem( m_remote, deviceId1, testItemName, CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    checkDeviceUpdateBroadcast( deviceId1, { { "armed", true } }, __LINE__ );

    std::string block;
    block = replaceTokensInString( isDeviceStateWithSourceWhenBlock,
                                   { { "__SOURCE_NAME__", "deviceGroup" },
                                     { "__SOURCE_TYPE__", "device_group" },
                                     { "__SOURCE_VALUE__", quote( deviceGroupId ) },
                                     { "__ARMED__", "false" },
                                     { "__REACHABLE__", "false" } } );

    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "isDeviceStateDeviceItemGroups" }, { "__WHEN_BLOCKS__", block }, { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    m_remote.setDeviceArmed( deviceId, false );
    checkDeviceUpdateBroadcast( deviceId, { { "armed", false } }, __LINE__ );

    m_remote.updateReachableState( deviceId, false );
    checkDeviceUpdateBroadcast( deviceId, { { "reachable", false } }, __LINE__ );

    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    m_remote.updateReachableState( deviceId1, false );
    checkDeviceUpdateBroadcast( deviceId1, { { "reachable", false } }, __LINE__ );

    m_remote.setDeviceArmed( deviceId1, false );
    checkDeviceUpdateBroadcast( deviceId1, { { "armed", false } }, __LINE__ );

    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    deleteScene( sceneId, __LINE__ );
    deleteItemGroup( itemGroupId, __LINE__ );
    deleteDeviceGroup( deviceGroupId, __LINE__ );
}

TEST_F( SceneDeviceItemGroupEventsTests, stringOperation_contain ) {  // 6.1
    const std::string deviceCategory = "security_sensor";
    const std::string deviceSubcategory = "motion";
    const std::string testItemName = "testItemName";

    auto createGroupRequest = Json::parse( scenesDeviceGroupCreate );
    createGroupRequest[ "params" ][ "name" ] = "stringOperation_contain";
    createGroupRequest[ "params" ][ "categories" ] =
        JSON_ARRAY( JSON( "category" << deviceCategory << "subcategories" << JSON_ARRAY( deviceSubcategory ) ) );
    auto deviceGroupId = createDeviceGroup( createGroupRequest, __LINE__ );

    // create item group
    // DONE add item groups when they will be implemented
    auto createItemGroupRequest = replaceTokens(
        scenesItemGroupCreate,
        { { "__NAME__", "testItemGroup" }, { "__ITEM_NAME__", testItemName }, { "__VALUE_TYPE__", "string" } } );
    json_remove( createItemGroupRequest[ "params" ], "enum" );

    auto itemGroupId = createItemGroup( createItemGroupRequest, __LINE__ );

    auto deviceId = makeDevice( deviceCategory, deviceSubcategory, __LINE__ );
    auto itemId = makeStringItem( deviceId, testItemName, "", __LINE__ );

    auto block = replaceTokensInString( itemOperationWhenBlock,
                                        { { "__METHOD__", "stringOperation" },
                                          { "__SOURCE_NAME__", "deviceGroup" },
                                          { "__SOURCE_TYPE__", "device_group" },
                                          { "__SOURCE_VALUE__", quote( deviceGroupId ) },
                                          { "__OPERATION__", "contain" },
                                          { "__TYPE__", "string" },
                                          { "__VALUE__", R"("testing")" } } );

    // add item group to args and fields
    auto fieldItemGroupSource = replaceTokens( sceneEventFieldsSource,
                                               { { "__SOURCE_NAME__", "itemGroup" },
                                                 { "__SOURCE_TYPE__", "item_group" },
                                                 { "__SOURCE_VALUE__", quote( itemGroupId ) } } );

    Json whenBlock = Json::parse( block );
    // appendArgument
    whenBlock[ "blockOptions" ][ "method" ][ "args" ][ "itemGroup" ] = "itemGroup";
    // appendField
    whenBlock[ "fields" ].push_back( fieldItemGroupSource );

    auto request = replaceTokens( createSceneRequest,
                                  { { "__NAME__", "stringOperationDeviceItemGroups" },
                                    { "__WHEN_BLOCKS__", whenBlock.dump() },
                                    { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    // the scene should be started by stringOperation block
    setItemValue( m_remote, itemId, "testing1", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    // stringOperationEvent should be stateless, scene should be triggered again
    setItemValue( m_remote, itemId, "testing2", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    deleteScene( sceneId, __LINE__ );
    deleteItemGroup( itemGroupId, __LINE__ );
    deleteDeviceGroup( deviceGroupId, __LINE__ );
}

TEST_F( SceneDeviceItemGroupEventsTests, inArray_in ) {  // 7.1
    const std::string deviceCategory = "security_sensor";
    const std::string deviceSubcategory = "motion";
    const std::string testItemName = "testItemName";

    auto createGroupRequest = Json::parse( scenesDeviceGroupCreate );
    createGroupRequest[ "params" ][ "name" ] = "inArray_in";
    createGroupRequest[ "params" ][ "categories" ] =
        JSON_ARRAY( JSON( "category" << deviceCategory << "subcategories" << JSON_ARRAY( deviceSubcategory ) ) );
    auto deviceGroupId = createDeviceGroup( createGroupRequest, __LINE__ );

    // create item group
    // DONE add item groups when they will be implemented
    auto createItemGroupRequest = replaceTokens(
        scenesItemGroupCreate,
        { { "__NAME__", "testItemGroup" }, { "__ITEM_NAME__", testItemName }, { "__VALUE_TYPE__", "int" } } );
    json_remove( createItemGroupRequest[ "params" ], "enum" );

    auto itemGroupId = createItemGroup( createItemGroupRequest, __LINE__ );

    auto deviceId = makeDevice( deviceCategory, deviceSubcategory, __LINE__ );
    auto itemId = makeIntItem( deviceId, testItemName, 0, __LINE__ );

    auto block = replaceTokensInString( itemOperationWhenBlock,
                                        { { "__METHOD__", "inArray" },
                                          { "__SOURCE_NAME__", "deviceGroup" },
                                          { "__SOURCE_TYPE__", "device_group" },
                                          { "__SOURCE_VALUE__", quote( deviceGroupId ) },
                                          { "__OPERATION__", "in" },
                                          { "__TYPE__", "array.int" },
                                          { "__VALUE__", "[1, 2, 3]" } } );

    // add item group to args and fields
    auto fieldItemGroupSource = replaceTokens( sceneEventFieldsSource,
                                               { { "__SOURCE_NAME__", "itemGroup" },
                                                 { "__SOURCE_TYPE__", "item_group" },
                                                 { "__SOURCE_VALUE__", quote( itemGroupId ) } } );

    Json whenBlock = Json::parse( block );
    // appendArgument
    whenBlock[ "blockOptions" ][ "method" ][ "args" ][ "itemGroup" ] = "itemGroup";
    // appendField
    whenBlock[ "fields" ].push_back( fieldItemGroupSource );

    auto request = replaceTokens( createSceneRequest,
                                  { { "__NAME__", "inArrayDeviceItemGroups" },
                                    { "__WHEN_BLOCKS__", whenBlock.dump() },
                                    { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    // the scene should be started by inArray block
    setItemValue( m_remote, itemId, 1, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    // inArrayEvent should be stateless, scene should be triggered again
    setItemValue( m_remote, itemId, 3, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    deleteScene( sceneId, __LINE__ );
    deleteItemGroup( itemGroupId, __LINE__ );
    deleteDeviceGroup( deviceGroupId, __LINE__ );
}

TEST_F( SceneDeviceItemGroupEventsTests, compareNumberRange ) {  // 8.1
    const std::string deviceCategory = "security_sensor";
    const std::string deviceSubcategory = "motion";
    const std::string testItemName = "testItemName";

    auto createGroupRequest = Json::parse( scenesDeviceGroupCreate );
    createGroupRequest[ "params" ][ "name" ] = "compareNumberRange";
    createGroupRequest[ "params" ][ "categories" ] =
        JSON_ARRAY( JSON( "category" << deviceCategory << "subcategories" << JSON_ARRAY( deviceSubcategory ) ) );
    auto deviceGroupId = createDeviceGroup( createGroupRequest, __LINE__ );

    // create item group
    // DONE add item groups when they will be implemented
    auto createItemGroupRequest = replaceTokens(
        scenesItemGroupCreate,
        { { "__NAME__", "testItemGroup" }, { "__ITEM_NAME__", testItemName }, { "__VALUE_TYPE__", "int" } } );
    json_remove( createItemGroupRequest[ "params" ], "enum" );

    auto itemGroupId = createItemGroup( createItemGroupRequest, __LINE__ );

    auto deviceId = makeDevice( deviceCategory, deviceSubcategory, __LINE__ );
    auto itemId = makeIntItem( deviceId, testItemName, 0, __LINE__ );

    auto block = replaceTokensInString( compareNumbersRangeConditionBlock,
                                        { { "__SOURCE_NAME__", "deviceGroup" },
                                          { "__SOURCE_TYPE__", "device_group" },
                                          { "__SOURCE_VALUE__", quote( deviceGroupId ) },
                                          { "__COMP__", "between" } } );

    // add item group to args and fields
    auto fieldItemGroupSource = replaceTokens( sceneEventFieldsSource,
                                               { { "__SOURCE_NAME__", "itemGroup" },
                                                 { "__SOURCE_TYPE__", "item_group" },
                                                 { "__SOURCE_VALUE__", quote( itemGroupId ) } } );

    Json whenBlock = Json::parse( block );
    // appendArgument
    whenBlock[ "blockOptions" ][ "method" ][ "args" ][ "itemGroup" ] = "itemGroup";
    // appendField
    whenBlock[ "fields" ].push_back( fieldItemGroupSource );

    auto request = replaceTokens( createSceneRequest,
                                  { { "__NAME__", "compareNumberRangeDeviceItemGroups" },
                                    { "__WHEN_BLOCKS__", whenBlock.dump() },
                                    { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    // the scene should be started by compareNumberRange block
    setItemValue( m_remote, itemId, 20, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    // compareNumberRangeEvent should be stateless, scene should be triggered again
    setItemValue( m_remote, itemId, 30, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    deleteScene( sceneId, __LINE__ );
    deleteItemGroup( itemGroupId, __LINE__ );
    deleteDeviceGroup( deviceGroupId, __LINE__ );
}

TEST_F( SceneDeviceItemGroupEventsTests, AndOperatorDeviceItemGroup ) {  // 9.1

    const std::string deviceCategory = "security_sensor";
    const std::string deviceSubcategory = "motion";
    const std::string testItemName = "testItemName";
    const std::string testItemName2 = "testItemName2";

    auto createGroupRequest = Json::parse( scenesDeviceGroupCreate );
    createGroupRequest[ "params" ][ "name" ] = "compareNumberRange";
    createGroupRequest[ "params" ][ "categories" ] =
        JSON_ARRAY( JSON( "category" << deviceCategory << "subcategories" << JSON_ARRAY( deviceSubcategory ) ) );
    auto deviceGroupId = createDeviceGroup( createGroupRequest, __LINE__ );

    // create item group
    // DONE add item groups when they will be implemented
    auto createItemGroupRequest = replaceTokens(
        scenesItemGroupCreate,
        { { "__NAME__", "testItemGroup" }, { "__ITEM_NAME__", testItemName }, { "__VALUE_TYPE__", "bool" } } );
    json_remove( createItemGroupRequest[ "params" ], "enum" );

    auto itemGroupId1 = createItemGroup( createItemGroupRequest, __LINE__ );

    createItemGroupRequest = replaceTokens(
        scenesItemGroupCreate,
        { { "__NAME__", "testItemGroup" }, { "__ITEM_NAME__", testItemName2 }, { "__VALUE_TYPE__", "bool" } } );
    json_remove( createItemGroupRequest[ "params" ], "enum" );

    auto itemGroupId2 = createItemGroup( createItemGroupRequest, __LINE__ );

    auto deviceId = makeDevice( deviceCategory, deviceSubcategory, __LINE__ );
    auto itemId1 = makeBooleanItem( deviceId, testItemName, false, __LINE__ );
    auto itemId2 = makeBooleanItem( deviceId, testItemName2, false, __LINE__ );

    auto whenBlock1 = replaceTokensInString( isItemStateWithSourceWhenBlock,
                                             { { "__SOURCE_NAME__", "itemGroup" },
                                               { "__SOURCE_TYPE__", "item_group" },
                                               { "__SOURCE_VALUE__", quote( itemGroupId1 ) },
                                               { "__TYPE__", "bool" },
                                               { "__VALUE__", "true" } } );
    auto whenBlock2 = replaceTokensInString( isItemStateWithSourceWhenBlock,
                                             { { "__SOURCE_NAME__", "itemGroup" },
                                               { "__SOURCE_TYPE__", "item_group" },
                                               { "__SOURCE_VALUE__", quote( itemGroupId2 ) },
                                               { "__TYPE__", "bool" },
                                               { "__VALUE__", "true" } } );
    whenBlock1 += ",";
    whenBlock1.append( std::move( whenBlock2 ) );
    auto andOperator = replaceTokensInString( andOperatorWhenBlock, { { "__BLOCKS__", whenBlock1 } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "AndOperator" }, { "__WHEN_BLOCKS__", andOperator }, { "__THEN_BLOCKS__", "" } } );
    SceneID sceneId = createScene( request );
    setItemValue( m_remote, itemId1, true, __LINE__ );
    setItemValue( m_remote, itemId2, true, __LINE__ );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    deleteScene( sceneId, __LINE__ );
    deleteItemGroup( itemGroupId1, __LINE__ );
    deleteItemGroup( itemGroupId2, __LINE__ );
    deleteDeviceGroup( deviceGroupId, __LINE__ );
}

TEST_F( SceneDeviceItemGroupEventsTests, isItemGroupStateSceneEdit ) {  // 10.1
    const std::string deviceCategory = "security_sensor";
    const std::string deviceSubcategory = "smoke";
    const std::string testItemName = "security_threat";

    // Set alarmDelay: 30 to 0
    makeRemote().setAlarmDelay( 0 );

    auto createGroupRequest = Json::parse( scenesDeviceGroupCreate );
    createGroupRequest[ "params" ][ "name" ] = "compareValue_exactHash";
    createGroupRequest[ "params" ][ "categories" ] =
        JSON_ARRAY( JSON( "category" << deviceCategory << "subcategories" << JSON_ARRAY( deviceSubcategory ) ) );
    auto deviceGroupId = createDeviceGroup( createGroupRequest, __LINE__ );
    auto createItemGroupRequest = replaceTokens(
        scenesItemGroupCreate,
        { { "__NAME__", "testItemGroup" }, { "__ITEM_NAME__", testItemName }, { "__VALUE_TYPE__", "bool" } } );
    json_remove( createItemGroupRequest[ "params" ], "enum" );

    auto itemGroupId = createItemGroup( createItemGroupRequest, __LINE__ );
    auto deviceId = makeDevice( deviceCategory, deviceSubcategory, __LINE__ );

    auto itemId =
        addItem( m_remote, deviceId, testItemName, CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    checkDeviceUpdateBroadcast( deviceId, { { "armed", true } }, __LINE__ );

    std::string block;
    block = replaceTokensInString( isItemStateWithSourceWhenBlock,
                                   { { "__SOURCE_NAME__", "deviceGroup" },
                                     { "__SOURCE_TYPE__", "device_group" },
                                     { "__SOURCE_VALUE__", quote( deviceGroupId ) },
                                     { "__TYPE__", "bool" },
                                     { "__VALUE__", "true" } } );

    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "isItemStateDeviceItemGroups" }, { "__WHEN_BLOCKS__", block }, { "__THEN_BLOCKS__", "" } } );

    SceneID sceneId = createScene( request, __LINE__ );
    LOGGER_TRACE( "Scene created, id: " + sceneId );

    setItemValue( m_remote, itemId, true, __LINE__ );
    checkBroadcasts(
        { [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, request, sceneId, "started" ); },
          [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, request, sceneId, "finished" ); },
          []( const Json& broadcast ) { return json_get_str( broadcast, "msg_subclass" ) == "hub.modes.alarmed"; } },
        __LINE__ );

    setItemValue( m_remote, itemId, false, __LINE__ );

    request = replaceTokens( editScenesRequest,
                             { { "__ID__", sceneId }, { "__WHEN_BLOCKS__", "" }, { "__THEN_BLOCKS__", "" } } );
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto broadcast = waitForBroadcast( "hub.scene.changed", "scene changed" );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.scene.changed" );

    setItemValue( m_remote, itemId, true, __LINE__ );

    checkBroadcasts(
        { []( const Json& broadcast ) { return json_get_str( broadcast, "msg_subclass" ) == "hub.modes.alarmed"; } },
        __LINE__ );

    deleteScene( sceneId, __LINE__ );
    deleteItemGroup( itemGroupId, __LINE__ );
    deleteDeviceGroup( deviceGroupId, __LINE__ );
}

}  // namespace Tests
}  // namespace HubPlatform
