#include "common_test_resources/fixtures/device_item_groups_fixture.h"
#include "common_test_resources/resources/scene_manager_resources.h"

#include <gtest/gtest.h>
#include "tiny/tiny_consolecolors.h"

namespace HubPlatform {
namespace Tests {

/*

TESTPLAN

DONE    1 Expand Device Item Group
DONE        1.1 hub.device.item.group.expand: passDeviceGroupIdOnlyWithShowItemsTrue
DONE        1.2 hub.device.item.group.expand: passDeviceGroupIdOnlyWithShowItemsFalse
DONE        1.3 hub.device.item.group.expand: passingVersion
DONE        1.4 hub.device.item.group.expand: passDeviceAndItemGroupId
DONE        1.5 hub.device.item.group.expand:errorReceiving

*/

class DeviceItemGroupExpandTests : public DeviceItemGroupsFixture {
public:
    void SetUp() override;

    void TearDown() override;
};

void DeviceItemGroupExpandTests::SetUp() {
    DeviceItemGroupsFixture::SetUp();
    clearExistingDevices( __LINE__ );
    callingFile( __FILE__ );
}

void DeviceItemGroupExpandTests::TearDown() {
    clearExistingDevices( __LINE__ );
    DeviceItemGroupsFixture::TearDown();
}

TEST_F( DeviceItemGroupExpandTests, passDeviceGroupIdOnlyWithShowItemsTrue ) {  // 1.1

    const std::string deviceCategory1 = "security_sensor";
    const std::string deviceSubcategory1 = "motion";
    const std::string deviceCategory2 = "scene_controller";
    const std::string deviceSubcategory2 = "panic";

    auto createGroupRequest = Json::parse( scenesDeviceGroupCreate );
    createGroupRequest[ "params" ][ "name" ] = "testingGroup";
    createGroupRequest[ "params" ][ "categories" ] =
        JSON_ARRAY( JSON( "category" << deviceCategory1 << "subcategories" << JSON_ARRAY( deviceSubcategory1 ) )
                    << JSON( "category" << deviceCategory2 << "subcategories" << JSON_ARRAY( deviceSubcategory2 ) ) );

    auto deviceGroupId = createDeviceGroup( createGroupRequest, __LINE__ );

    auto createItemGroupRequest = replaceTokens(
        scenesItemGroupCreate,
        { { "__NAME__", "testItemGroup" }, { "__ITEM_NAME__", "testItemName" }, { "__VALUE_TYPE__", "int" } } );
    json_remove( createItemGroupRequest[ "params" ], "enum" );

    auto itemGroupId = createItemGroup( createItemGroupRequest, __LINE__ );

    auto deviceId1 = makeDevice( deviceCategory1, deviceSubcategory1, __LINE__ );
    auto deviceId2 = makeDevice( deviceCategory1, deviceSubcategory1, __LINE__ );
    auto itemId1 = makeIntItem( deviceId1, "testItemName", 1, __LINE__ );
    auto itemId2 = makeIntItem( deviceId2, "testItemName", 2, __LINE__ );
    auto itemId3 = makeIntItem( deviceId1, "testItemName2", 3, __LINE__ );
    auto itemId4 = makeIntItem( deviceId2, "testItemName3", 4, __LINE__ );

    auto request = replaceTokensInString( deviceGroupExpand,
                                          { { "__DEVICE_GROUP_ID__", deviceGroupId }, { "__FLAG__", "true" } } );
    auto answer = nma().sendtoHubAndGetAnswer( Json::parse( request ) );
    const auto expandGroup =
        JSON( "devices" << JSON_ARRAY_( JSON( "deviceId" << deviceId1 << "items"
                                                         << JSON_ARRAY_( JSON( "itemId" << itemId1 << "name"
                                                                                        << "testItemName" ),
                                                                         JSON( "itemId" << itemId3 << "name"
                                                                                        << "testItemName2" ) )
                                                         << "name"
                                                         << "Controller device-item group device" ),
                                        JSON( "deviceId" << deviceId2 << "items"
                                                         << JSON_ARRAY_( JSON( "itemId" << itemId2 << "name"
                                                                                        << "testItemName" ),
                                                                         JSON( "itemId" << itemId4 << "name"
                                                                                        << "testItemName3" ) )
                                                         << "name"
                                                         << "Controller device-item group device" ) ) );

    EXPECT_TRUE( answer[ "error" ].empty() );
    auto devices = answer[ "result" ];
    EXPECT_EQ( devices[ "devices" ], expandGroup[ "devices" ] );
    deleteDeviceGroup( deviceGroupId, __LINE__ );
    deleteItemGroup( itemGroupId, __LINE__ );
}

TEST_F( DeviceItemGroupExpandTests, passDeviceGroupIdOnlyWithShowItemsFalse ) {  // 1.2

    const std::string deviceCategory1 = "security_sensor";
    const std::string deviceSubcategory1 = "motion";
    const std::string deviceCategory2 = "scene_controller";
    const std::string deviceSubcategory2 = "panic";

    auto createGroupRequest = Json::parse( scenesDeviceGroupCreate );
    createGroupRequest[ "params" ][ "name" ] = "testingGroup";
    createGroupRequest[ "params" ][ "categories" ] =
        JSON_ARRAY( JSON( "category" << deviceCategory1 << "subcategories" << JSON_ARRAY( deviceSubcategory1 ) )
                    << JSON( "category" << deviceCategory2 << "subcategories" << JSON_ARRAY( deviceSubcategory2 ) ) );

    auto deviceGroupId = createDeviceGroup( createGroupRequest, __LINE__ );

    auto createItemGroupRequest = replaceTokens(
        scenesItemGroupCreate,
        { { "__NAME__", "testItemGroup" }, { "__ITEM_NAME__", "testItemName" }, { "__VALUE_TYPE__", "int" } } );
    json_remove( createItemGroupRequest[ "params" ], "enum" );

    auto itemGroupId = createItemGroup( createItemGroupRequest, __LINE__ );

    auto deviceId1 = makeDevice( deviceCategory1, deviceSubcategory1, __LINE__ );
    auto deviceId2 = makeDevice( deviceCategory1, deviceSubcategory1, __LINE__ );
    auto itemId1 = makeIntItem( deviceId1, "testItemName", 1, __LINE__ );
    auto itemId2 = makeIntItem( deviceId2, "testItemName", 2, __LINE__ );
    auto itemId3 = makeIntItem( deviceId1, "testItemName2", 3, __LINE__ );
    auto itemId4 = makeIntItem( deviceId2, "testItemName3", 4, __LINE__ );

    auto request = replaceTokensInString( deviceGroupExpand,
                                          { { "__DEVICE_GROUP_ID__", deviceGroupId }, { "__FLAG__", "false" } } );
    auto answer = nma().sendtoHubAndGetAnswer( Json::parse( request ) );
    const auto expandGroup =
        JSON( "devices" << JSON_ARRAY_( JSON( "deviceId" << deviceId1 << "name"
                                                         << "Controller device-item group device" ),
                                        JSON( "deviceId" << deviceId2 << "name"
                                                         << "Controller device-item group device" ) ) );

    EXPECT_TRUE( answer[ "error" ].empty() );
    auto devices = answer[ "result" ];
    EXPECT_EQ( devices[ "devices" ], expandGroup[ "devices" ] );
    deleteDeviceGroup( deviceGroupId, __LINE__ );
    deleteItemGroup( itemGroupId, __LINE__ );
}

TEST_F( DeviceItemGroupExpandTests, passingVersion ) {  // 1.3

    const std::string deviceCategory1 = "security_sensor";
    const std::string deviceSubcategory1 = "motion";
    const std::string deviceCategory2 = "scene_controller";
    const std::string deviceSubcategory2 = "panic";

    auto createGroupRequest = Json::parse( scenesDeviceGroupCreate );
    createGroupRequest[ "params" ][ "name" ] = "testingGroup";
    createGroupRequest[ "params" ][ "categories" ] =
        JSON_ARRAY( JSON( "category" << deviceCategory1 << "subcategories" << JSON_ARRAY( deviceSubcategory1 ) )
                    << JSON( "category" << deviceCategory2 << "subcategories" << JSON_ARRAY( deviceSubcategory2 ) ) );

    auto deviceGroupId = createDeviceGroup( createGroupRequest, __LINE__ );

    auto createItemGroupRequest = replaceTokens(
        scenesItemGroupCreate,
        { { "__NAME__", "testItemGroup" }, { "__ITEM_NAME__", "testItemName" }, { "__VALUE_TYPE__", "int" } } );
    json_remove( createItemGroupRequest[ "params" ], "enum" );

    auto itemGroupId = createItemGroup( createItemGroupRequest, __LINE__ );

    auto deviceId1 = makeDevice( deviceCategory1, deviceSubcategory1, __LINE__ );
    auto deviceId2 = makeDevice( deviceCategory1, deviceSubcategory1, __LINE__ );
    auto itemId1 = makeIntItem( deviceId1, "testItemName", 1, __LINE__ );
    auto itemId2 = makeIntItem( deviceId2, "testItemName", 2, __LINE__ );
    auto itemId3 = makeIntItem( deviceId1, "testItemName2", 3, __LINE__ );
    auto itemId4 = makeIntItem( deviceId2, "testItemName3", 4, __LINE__ );

    auto request = replaceTokensInString( deviceGroupExpand,
                                          { { "__DEVICE_GROUP_ID__", deviceGroupId }, { "__FLAG__", "false" } } );
    auto answer = nma().sendtoHubAndGetAnswer( Json::parse( request ) );
    const auto expandGroup =
        JSON( "devices" << JSON_ARRAY_( JSON( "deviceId" << deviceId1 << "name"
                                                         << "Controller device-item group device" ),
                                        JSON( "deviceId" << deviceId2 << "name"
                                                         << "Controller device-item group device" ) ) );

    EXPECT_TRUE( answer[ "error" ].empty() );
    auto result = answer[ "result" ];
    EXPECT_EQ( result[ "devices" ], expandGroup[ "devices" ] );

    auto version = json_get_str( result, "version" );
    request = replaceTokensInString(
        deviceGroupExpand,
        { { "__DEVICE_GROUP_ID__", deviceGroupId }, { "__FLAG__", "false" }, { "__VERSION__", version } } );
    auto response = nma().sendtoHubAndGetAnswer( Json::parse( request ) );
    EXPECT_TRUE( response[ "error" ].empty() );
    result = response[ "result" ];
    EXPECT_TRUE( result[ "devices" ].empty() );
    EXPECT_FALSE( result[ "version" ].empty() );
    EXPECT_EQ( result[ "version" ], version );

    deleteDeviceGroup( deviceGroupId, __LINE__ );
    deleteItemGroup( itemGroupId, __LINE__ );
}

TEST_F( DeviceItemGroupExpandTests, passDeviceAndItemGroupId ) {  // 1.4

    const std::string deviceCategory1 = "security_sensor";
    const std::string deviceSubcategory1 = "motion";
    const std::string deviceCategory2 = "scene_controller";
    const std::string deviceSubcategory2 = "panic";

    auto createGroupRequest = Json::parse( scenesDeviceGroupCreate );
    createGroupRequest[ "params" ][ "name" ] = "testingGroup";
    createGroupRequest[ "params" ][ "categories" ] =
        JSON_ARRAY( JSON( "category" << deviceCategory1 << "subcategories" << JSON_ARRAY( deviceSubcategory1 ) )
                    << JSON( "category" << deviceCategory2 << "subcategories" << JSON_ARRAY( deviceSubcategory2 ) ) );

    auto deviceGroupId = createDeviceGroup( createGroupRequest, __LINE__ );

    auto createItemGroupRequest = replaceTokens(
        scenesItemGroupCreate,
        { { "__NAME__", "testItemGroup" }, { "__ITEM_NAME__", "testItemName" }, { "__VALUE_TYPE__", "int" } } );
    json_remove( createItemGroupRequest[ "params" ], "enum" );

    auto itemGroupId = createItemGroup( createItemGroupRequest, __LINE__ );

    auto deviceId1 = makeDevice( deviceCategory1, deviceSubcategory1, __LINE__ );
    auto deviceId2 = makeDevice( deviceCategory1, deviceSubcategory1, __LINE__ );
    auto itemId1 = makeIntItem( deviceId1, "testItemName", 1, __LINE__ );
    auto itemId2 = makeIntItem( deviceId2, "testItemName", 2, __LINE__ );
    auto itemId3 = makeIntItem( deviceId1, "testItemName2", 3, __LINE__ );
    auto itemId4 = makeIntItem( deviceId2, "testItemName3", 4, __LINE__ );

    auto request = replaceTokensInString(
        deviceItemGroupExpand, { { "__DEVICE_GROUP_ID__", deviceGroupId }, { "__ITEM_GROUP_ID__", itemGroupId } } );
    auto answer = nma().sendtoHubAndGetAnswer( Json::parse( request ) );
    const auto expandGroup =
        JSON( "devices" << JSON_ARRAY_( JSON( "deviceId" << deviceId1 << "items"
                                                         << JSON_ARRAY_( JSON( "itemId" << itemId1 << "name"
                                                                                        << "testItemName" ) )
                                                         << "name"
                                                         << "Controller device-item group device" ),
                                        JSON( "deviceId" << deviceId2 << "items"
                                                         << JSON_ARRAY_( JSON( "itemId" << itemId2 << "name"
                                                                                        << "testItemName" ) )
                                                         << "name"
                                                         << "Controller device-item group device" ) ) );

    EXPECT_TRUE( answer[ "error" ].empty() );
    auto devices = answer[ "result" ];
    EXPECT_EQ( devices[ "devices" ], expandGroup[ "devices" ] );
    deleteDeviceGroup( deviceGroupId, __LINE__ );
    deleteItemGroup( itemGroupId, __LINE__ );
}

TEST_F( DeviceItemGroupExpandTests, errorReceiving ) {  // 1.5

    const std::string deviceCategory1 = "security_sensor";
    const std::string deviceSubcategory1 = "motion";
    const std::string deviceCategory2 = "scene_controller";
    const std::string deviceSubcategory2 = "panic";

    auto createItemGroupRequest = replaceTokens(
        scenesItemGroupCreate,
        { { "__NAME__", "testItemGroup" }, { "__ITEM_NAME__", "testItemName" }, { "__VALUE_TYPE__", "int" } } );
    json_remove( createItemGroupRequest[ "params" ], "enum" );

    auto itemGroupId = createItemGroup( createItemGroupRequest, __LINE__ );

    auto deviceId1 = makeDevice( deviceCategory1, deviceSubcategory1, __LINE__ );
    auto deviceId2 = makeDevice( deviceCategory1, deviceSubcategory1, __LINE__ );
    auto itemId1 = makeIntItem( deviceId1, "testItemName", 1, __LINE__ );
    auto itemId2 = makeIntItem( deviceId2, "testItemName", 2, __LINE__ );
    auto itemId3 = makeIntItem( deviceId1, "testItemName2", 3, __LINE__ );
    auto itemId4 = makeIntItem( deviceId2, "testItemName3", 4, __LINE__ );

    auto request = replaceTokensInString(
        deviceItemGroupExpand, { { "__DEVICE_GROUP_ID__", "0000000" }, { "__ITEM_GROUP_ID__", itemGroupId } } );
    auto answer = nma().sendtoHubAndGetAnswer( Json::parse( request ) );

    EXPECT_FALSE( answer[ "error" ].empty() );
    auto error = answer[ "error" ];

    EXPECT_EQ( error[ "data" ], "ezlo.not.exist" );
    EXPECT_EQ( error[ "message" ], "Record with the key does not exist" );
    EXPECT_EQ( error[ "reason" ], "Device group doesn't exist" );

    deleteItemGroup( itemGroupId, __LINE__ );
}

}  // namespace Tests
}  // namespace HubPlatform