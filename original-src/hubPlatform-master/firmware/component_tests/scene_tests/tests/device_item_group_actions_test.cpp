#include "common_test_resources/fixtures/device_item_groups_fixture.h"
#include "common_test_resources/resources/scene_manager_resources.h"
#include "common_test_resources/resources/cored_resources.h"

#include <gtest/gtest.h>
#include "tiny/tiny_consolecolors.h"
#include "cored/remote_interface/cored_remote_interface.h"

namespace HubPlatform {
namespace Tests {

/*

TESTPLAN

DONE    1 Group Item Value Set Operation
DONE        1.1 groupSetItemValue: oneDeviceMultipleItems
DONE        1.2 groupSetItemValue: multipleDevicesMultipleItems
DONE        1.3 groupSetItemValue: oneDeviceNotPartOfDeviceGroup
DONE        1.4 groupSetItemValue: withDevicesAndExceptionObjects
DONE    2 Group Device Armed Operation
DONE        2.1 groupSetDeviceArmed
DONE    3 Group Item Value Toggle Operation
DONE        3.1 groupToggleValue

*/

class SceneDeviceItemGroupActionsTests : public DeviceItemGroupsFixture {
public:
    void SetUp() override;

    void TearDown() override;
};

void SceneDeviceItemGroupActionsTests::SetUp() {
    DeviceItemGroupsFixture::SetUp();
    clearExistingDevices( __LINE__ );
    callingFile( __FILE__ );
}

void SceneDeviceItemGroupActionsTests::TearDown() {
    clearExistingDevices( __LINE__ );
    DeviceItemGroupsFixture::TearDown();
}

TEST_F( SceneDeviceItemGroupActionsTests, oneDeviceMultipleItems ) {  // 1.1

    const std::string deviceCategory = "security_sensor";
    const std::string deviceSubcategory = "motion";
    const std::string testItemName1 = "testItemName1";
    const std::string testItemName2 = "testItemName2";
    const std::string testItemName3 = "testItemName3";
    const std::string testItemName4 = "testItemName4";

    auto createGroupRequest = Json::parse( scenesDeviceGroupCreate );
    createGroupRequest[ "params" ][ "name" ] = "testingGroup";
    createGroupRequest[ "params" ][ "categories" ] =
        JSON_ARRAY( JSON( "category" << deviceCategory << "subcategories" << JSON_ARRAY( deviceSubcategory ) ) );

    auto deviceGroupId = createDeviceGroup( createGroupRequest, __LINE__ );

    auto createItemGroupRequest = replaceTokens( scenesItemGroupCreateWithMultipleItemNames,
                                                 { { "__NAME__", "testItemGroup" },
                                                   { "__ITEM_NAME1__", testItemName1 },
                                                   { "__ITEM_NAME2__", testItemName2 },
                                                   { "__ITEM_NAME3__", testItemName3 },
                                                   { "__ITEM_NAME4__", testItemName4 },
                                                   { "__VALUE_TYPE__", "int" } } );
    json_remove( createItemGroupRequest[ "params" ], "enum" );

    auto itemGroupId = createItemGroup( createItemGroupRequest, __LINE__ );

    auto deviceId1 = makeDevice( deviceCategory, deviceSubcategory, __LINE__ );
    auto itemId1 = makeIntItem( deviceId1, testItemName1, 1, __LINE__ );
    auto itemId2 = makeIntItem( deviceId1, testItemName2, 2, __LINE__ );
    auto itemId3 = makeIntItem( deviceId1, testItemName3, 3, __LINE__ );
    auto itemId4 = makeIntItem( deviceId1, testItemName4, 4, __LINE__ );

    auto request = createEmptyScene;
    request[ "params" ][ "then" ].push_back( replaceTokens( groupSetItemValueThenBlock,
                                                            { { "__GROUP_DEVICE_ID__", deviceGroupId },
                                                              { "__GROUP_ITEM_ID__", itemGroupId },
                                                              { "__TYPE__", "int" },
                                                              { "__VALUE__", "10" } } ) );

    SceneID sceneId = createScene( request, __LINE__ );

    runSceneManually( sceneId, __LINE__ );

    checkBroadcasts(
        { [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, request, sceneId, "started" ); },
          [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, request, sceneId, "finished" ); },
          [this, itemId1]( const Json& broadcast ) { return isItemUpdateBroadcast( broadcast, itemId1, 10 ); },
          [this, itemId2]( const Json& broadcast ) { return isItemUpdateBroadcast( broadcast, itemId2, 10 ); },
          [this, itemId3]( const Json& broadcast ) { return isItemUpdateBroadcast( broadcast, itemId3, 10 ); },
          [this, itemId4]( const Json& broadcast ) { return isItemUpdateBroadcast( broadcast, itemId4, 10 ); } },
        __LINE__ );

    deleteDeviceGroup( deviceGroupId, __LINE__ );
    deleteItemGroup( itemGroupId, __LINE__ );
}

TEST_F( SceneDeviceItemGroupActionsTests, multipleDevicesMultipleItems ) {  // 1.2

    const std::string deviceCategory1 = "security_sensor";
    const std::string deviceSubcategory1 = "motion";
    const std::string deviceCategory2 = "scene_controller";
    const std::string deviceSubcategory2 = "panic";
    const std::string testItemName = "testItemName";

    auto createGroupRequest = Json::parse( scenesDeviceGroupCreate );
    createGroupRequest[ "params" ][ "name" ] = "testingGroup";
    createGroupRequest[ "params" ][ "categories" ] =
        JSON_ARRAY( JSON( "category" << deviceCategory1 << "subcategories" << JSON_ARRAY( deviceSubcategory1 ) )
                    << JSON( "category" << deviceCategory2 << "subcategories" << JSON_ARRAY( deviceSubcategory2 ) ) );

    auto deviceGroupId = createDeviceGroup( createGroupRequest, __LINE__ );

    auto createItemGroupRequest = replaceTokens(
        scenesItemGroupCreate,
        { { "__NAME__", "testItemGroup" }, { "__ITEM_NAME__", testItemName }, { "__VALUE_TYPE__", "int" } } );
    json_remove( createItemGroupRequest[ "params" ], "enum" );

    auto itemGroupId = createItemGroup( createItemGroupRequest, __LINE__ );

    auto deviceId1 = makeDevice( deviceCategory1, deviceSubcategory1, __LINE__ );
    auto deviceId2 = makeDevice( deviceCategory1, deviceSubcategory1, __LINE__ );
    auto deviceId3 = makeDevice( deviceCategory2, deviceSubcategory2, __LINE__ );
    auto deviceId4 = makeDevice( deviceCategory2, deviceSubcategory2, __LINE__ );
    auto itemId1 = makeIntItem( deviceId1, testItemName, 1, __LINE__ );
    auto itemId2 = makeIntItem( deviceId2, testItemName, 2, __LINE__ );
    auto itemId3 = makeIntItem( deviceId3, testItemName, 3, __LINE__ );
    auto itemId4 = makeIntItem( deviceId4, testItemName, 4, __LINE__ );

    auto request = createEmptyScene;
    request[ "params" ][ "then" ].push_back( replaceTokens( groupSetItemValueThenBlock,
                                                            { { "__GROUP_DEVICE_ID__", deviceGroupId },
                                                              { "__GROUP_ITEM_ID__", itemGroupId },
                                                              { "__TYPE__", "int" },
                                                              { "__VALUE__", "10" } } ) );

    SceneID sceneId = createScene( request, __LINE__ );

    runSceneManually( sceneId, __LINE__ );

    checkBroadcasts(
        { [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, request, sceneId, "started" ); },
          [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, request, sceneId, "finished" ); },
          [this, itemId1]( const Json& broadcast ) { return isItemUpdateBroadcast( broadcast, itemId1, 10 ); },
          [this, itemId2]( const Json& broadcast ) { return isItemUpdateBroadcast( broadcast, itemId2, 10 ); },
          [this, itemId3]( const Json& broadcast ) { return isItemUpdateBroadcast( broadcast, itemId3, 10 ); },
          [this, itemId4]( const Json& broadcast ) { return isItemUpdateBroadcast( broadcast, itemId4, 10 ); } },
        __LINE__ );

    deleteDeviceGroup( deviceGroupId, __LINE__ );
    deleteItemGroup( itemGroupId, __LINE__ );
}

TEST_F( SceneDeviceItemGroupActionsTests, oneDeviceNotPartOfDeviceGroup ) {  // 1.3

    auto& coredRemote = makeRemote();
    const std::string deviceCategory = "security_sensor";
    const std::string deviceSubcategory1 = "motion1";
    const std::string deviceSubcategory2 = "motion2";
    const std::string deviceSubcategory3 = "motion3";
    const std::string testItemName1 = "testItemName1";
    const std::string testItemName2 = "testItemName2";
    const std::string testItemName3 = "testItemName3";
    const std::string testItemName4 = "testItemName4";
    const int newValue = 10;

    auto createGroupRequest = Json::parse( scenesDeviceGroupCreate );
    createGroupRequest[ "params" ][ "name" ] = "testingGroup";
    createGroupRequest[ "params" ][ "categories" ] = JSON_ARRAY(
        JSON( "category" << deviceCategory << "subcategories"
                         << JSON_ARRAY( deviceSubcategory1 << deviceSubcategory2 << deviceSubcategory3 ) ) );

    auto deviceGroupId = createDeviceGroup( createGroupRequest, __LINE__ );

    auto createItemGroupRequest = replaceTokens( scenesItemGroupCreateWithMultipleItemNames,
                                                 { { "__NAME__", "testItemGroup" },
                                                   { "__ITEM_NAME1__", testItemName1 },
                                                   { "__ITEM_NAME2__", testItemName2 },
                                                   { "__ITEM_NAME3__", testItemName3 },
                                                   { "__ITEM_NAME4__", testItemName4 },
                                                   { "__VALUE_TYPE__", "int" } } );
    json_remove( createItemGroupRequest[ "params" ], "enum" );

    auto itemGroupId = createItemGroup( createItemGroupRequest, __LINE__ );

    auto deviceId1 = makeDevice( deviceCategory, deviceSubcategory1, __LINE__ );
    auto deviceId2 = makeDevice( deviceCategory, deviceSubcategory2, __LINE__ );
    auto deviceId3 = makeDevice( deviceCategory, deviceSubcategory3, __LINE__ );
    auto deviceId4 = makeDevice( deviceCategory, "notInDeviceGroup", __LINE__ );
    auto itemId1 = makeIntItem( deviceId1, testItemName1, 1, __LINE__ );
    auto itemId2 = makeIntItem( deviceId2, testItemName2, 2, __LINE__ );
    auto itemId3 = makeIntItem( deviceId3, testItemName3, 3, __LINE__ );
    auto itemId4 = makeIntItem( deviceId4, testItemName4, 4, __LINE__ );

    auto request = createEmptyScene;
    request[ "params" ][ "then" ].push_back( replaceTokens( groupSetItemValueThenBlock,
                                                            { { "__GROUP_DEVICE_ID__", deviceGroupId },
                                                              { "__GROUP_ITEM_ID__", itemGroupId },
                                                              { "__TYPE__", "int" },
                                                              { "__VALUE__", std::to_string( newValue ) } } ) );

    SceneID sceneId = createScene( request, __LINE__ );

    runSceneManually( sceneId, __LINE__ );

    checkBroadcasts(
        { [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, request, sceneId, "started" ); },
          [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, request, sceneId, "finished" ); },
          [this, itemId1]( const Json& broadcast ) { return isItemUpdateBroadcast( broadcast, itemId1, newValue ); },
          [this, itemId2]( const Json& broadcast ) { return isItemUpdateBroadcast( broadcast, itemId2, newValue ); },
          [this, itemId3]( const Json& broadcast ) { return isItemUpdateBroadcast( broadcast, itemId3, newValue ); } },
        __LINE__ );

    auto item4 = coredRemote.getItem( itemId4 );
    EXPECT_NE( json_get_int( item4, "value" ), newValue );

    deleteDeviceGroup( deviceGroupId, __LINE__ );
    deleteItemGroup( itemGroupId, __LINE__ );
}

TEST_F( SceneDeviceItemGroupActionsTests, withDevicesAndExceptionObjects ) {  // 1.4

    auto& coredRemote = makeRemote();
    const std::string deviceCategory = "security_sensor";
    const std::string deviceSubcategory1 = "motion1";
    const std::string deviceSubcategory2 = "motion2";
    const std::string testItemName1 = "testItemName1";
    const std::string testItemName2 = "testItemName2";
    const std::string testItemName3 = "testItemName3";
    const std::string testItemName4 = "testItemName4";
    const int newValue = 10;

    auto deviceId1 = makeDevice( "testDeviceCategory", "testDeviceSubcategory", __LINE__ );
    auto deviceId2 = makeDevice( deviceCategory, deviceSubcategory2, __LINE__ );

    auto createGroupRequest = Json::parse( scenesDeviceGroupCreate );
    createGroupRequest[ "params" ][ "name" ] = "testingGroup";
    createGroupRequest[ "params" ][ "categories" ] = JSON_ARRAY( JSON(
        "category" << deviceCategory << "subcategories" << JSON_ARRAY( deviceSubcategory1 << deviceSubcategory2 ) ) );
    createGroupRequest[ "params" ][ "devices" ] = JSON_ARRAY( deviceId1 );
    createGroupRequest[ "params" ][ "exceptions" ] = JSON_ARRAY( deviceId2 );

    auto deviceGroupId = createDeviceGroup( createGroupRequest, __LINE__ );

    auto createItemGroupRequest = replaceTokens( scenesItemGroupCreateWithMultipleItemNames,
                                                 { { "__NAME__", "testItemGroup" },
                                                   { "__ITEM_NAME1__", testItemName1 },
                                                   { "__ITEM_NAME2__", testItemName2 },
                                                   { "__ITEM_NAME3__", testItemName3 },
                                                   { "__ITEM_NAME4__", testItemName4 },
                                                   { "__VALUE_TYPE__", "int" } } );
    json_remove( createItemGroupRequest[ "params" ], "enum" );

    auto itemGroupId = createItemGroup( createItemGroupRequest, __LINE__ );

    auto deviceId3 = makeDevice( deviceCategory, deviceSubcategory1, __LINE__ );
    auto deviceId4 = makeDevice( deviceCategory, deviceSubcategory2, __LINE__ );
    auto itemId1 = makeIntItem( deviceId1, testItemName1, 1, __LINE__ );
    auto itemId2 = makeIntItem( deviceId2, testItemName2, 2, __LINE__ );
    auto itemId3 = makeIntItem( deviceId3, testItemName3, 3, __LINE__ );
    auto itemId4 = makeIntItem( deviceId4, testItemName4, 4, __LINE__ );

    auto request = createEmptyScene;
    request[ "params" ][ "then" ].push_back( replaceTokens( groupSetItemValueThenBlock,
                                                            { { "__GROUP_DEVICE_ID__", deviceGroupId },
                                                              { "__GROUP_ITEM_ID__", itemGroupId },
                                                              { "__TYPE__", "int" },
                                                              { "__VALUE__", std::to_string( newValue ) } } ) );

    SceneID sceneId = createScene( request, __LINE__ );

    runSceneManually( sceneId, __LINE__ );

    checkBroadcasts(
        { [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, request, sceneId, "started" ); },
          [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, request, sceneId, "finished" ); },
          [this, itemId1]( const Json& broadcast ) { return isItemUpdateBroadcast( broadcast, itemId1, newValue ); },
          [this, itemId3]( const Json& broadcast ) { return isItemUpdateBroadcast( broadcast, itemId3, newValue ); },
          [this, itemId4]( const Json& broadcast ) { return isItemUpdateBroadcast( broadcast, itemId4, newValue ); } },
        __LINE__ );

    auto item2 = coredRemote.getItem( itemId2 );
    EXPECT_NE( json_get_int( item2, "value" ), newValue );

    deleteDeviceGroup( deviceGroupId, __LINE__ );
    deleteItemGroup( itemGroupId, __LINE__ );
}

TEST_F( SceneDeviceItemGroupActionsTests, groupSetDeviceArmed ) {  // 2.1

    auto& coreRemoteInterface = makeRemote();
    const std::string deviceCategory = "security_sensor";
    const std::string deviceSubcategory1 = "smoke";
    const std::string deviceSubcategory3 = "co";

    auto createGroupRequest = Json::parse( scenesDeviceGroupCreate );
    createGroupRequest[ "params" ][ "name" ] = "testingGroup";
    createGroupRequest[ "params" ][ "categories" ] = JSON_ARRAY( JSON(
        "category" << deviceCategory << "subcategories" << JSON_ARRAY( deviceSubcategory1 << deviceSubcategory3 ) ) );

    auto deviceGroupId = createDeviceGroup( createGroupRequest, __LINE__ );

    auto deviceId1 = createSecuritySensorDevice( "testDevice1", "smoke" );
    auto deviceId3 = createSecuritySensorDevice( "testDevice3", "co" );

    using namespace CoreD::Model::Resources;
    // set armed
    coreRemoteInterface.setDeviceArmed( deviceId1, false );
    checkDeviceUpdateBroadcast( deviceId1, { { DeviceFields::ArmedFieldName, false } }, __LINE__ );
    coreRemoteInterface.setDeviceArmed( deviceId3, false );
    checkDeviceUpdateBroadcast( deviceId3, { { DeviceFields::ArmedFieldName, false } }, __LINE__ );

    auto request = createEmptyScene;
    request[ "params" ][ "then" ].push_back( replaceTokens(
        groupSetDeviceArmedThenBlock, { { "__DEVICE_GROUP_ID__", deviceGroupId }, { "__VALUE__", "true" } } ) );

    SceneID sceneId = createScene( request, __LINE__ );

    runSceneManually( sceneId, __LINE__ );

    checkBroadcasts(
        { [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, request, sceneId, "started" ); },
          [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, request, sceneId, "finished" ); },
          [this, deviceId1]( const Json& broadcast ) {
              return isDeviceUpdateBroadcast( broadcast, deviceId1, "true" );
          },
          [this, deviceId3]( const Json& broadcast ) {
              return isDeviceUpdateBroadcast( broadcast, deviceId3, "true" );
          } },
        __LINE__ );

    deleteDeviceGroup( deviceGroupId, __LINE__ );
}

TEST_F( SceneDeviceItemGroupActionsTests, groupToggleValue ) {  // 3.1

    const std::string deviceCategory = "security_sensor";
    const std::string deviceSubcategory = "motion";
    const std::string testItemName1 = "testItemName1";
    const std::string testItemName2 = "testItemName2";
    const std::string testItemName3 = "testItemName3";
    const std::string testItemName4 = "testItemName4";

    auto createGroupRequest = Json::parse( scenesDeviceGroupCreate );
    createGroupRequest[ "params" ][ "name" ] = "testingGroup";
    createGroupRequest[ "params" ][ "categories" ] =
        JSON_ARRAY( JSON( "category" << deviceCategory << "subcategories" << JSON_ARRAY( deviceSubcategory ) ) );

    auto deviceGroupId = createDeviceGroup( createGroupRequest, __LINE__ );

    auto createItemGroupRequest = replaceTokens( scenesItemGroupCreateWithMultipleItemNames,
                                                 { { "__NAME__", "testItemGroup" },
                                                   { "__ITEM_NAME1__", testItemName1 },
                                                   { "__ITEM_NAME2__", testItemName2 },
                                                   { "__ITEM_NAME3__", testItemName3 },
                                                   { "__ITEM_NAME4__", testItemName4 },
                                                   { "__VALUE_TYPE__", "bool" } } );
    json_remove( createItemGroupRequest[ "params" ], "enum" );

    auto itemGroupId = createItemGroup( createItemGroupRequest, __LINE__ );

    auto deviceId1 = makeDevice( deviceCategory, deviceSubcategory, __LINE__ );
    auto itemId1 = makeBooleanItem( deviceId1, testItemName1, true, __LINE__ );
    auto itemId2 = makeBooleanItem( deviceId1, testItemName2, true, __LINE__ );
    auto itemId3 = makeBooleanItem( deviceId1, testItemName3, false, __LINE__ );
    auto itemId4 = makeBooleanItem( deviceId1, testItemName4, false, __LINE__ );

    auto request = createEmptyScene;
    request[ "params" ][ "then" ].push_back(
        replaceTokens( groupToggleValueThenBlock,
                       { { "__GROUP_DEVICE_ID__", deviceGroupId }, { "__GROUP_ITEM_ID__", itemGroupId } } ) );

    SceneID sceneId = createScene( request, __LINE__ );

    runSceneManually( sceneId, __LINE__ );

    checkBroadcasts(
        { [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, request, sceneId, "started" ); },
          [&]( const Json& broadcast ) { return isRunProgressBroadcast( broadcast, request, sceneId, "finished" ); },
          [this, itemId1]( const Json& broadcast ) { return isItemUpdateBroadcast( broadcast, itemId1, false ); },
          [this, itemId2]( const Json& broadcast ) { return isItemUpdateBroadcast( broadcast, itemId2, false ); },
          [this, itemId3]( const Json& broadcast ) { return isItemUpdateBroadcast( broadcast, itemId3, true ); },
          [this, itemId4]( const Json& broadcast ) { return isItemUpdateBroadcast( broadcast, itemId4, true ); } },
        __LINE__ );

    deleteScene( sceneId, __LINE__ );
    deleteDeviceGroup( deviceGroupId, __LINE__ );
    deleteItemGroup( itemGroupId, __LINE__ );
}

}  // namespace Tests
}  // namespace HubPlatform