
#include "cored/group_operations/device_item_group_manager.h"

#include <set>
#include "json_utils/json_utils.h"
#include <gtest/gtest.h>

namespace HubPlatform {

namespace UT {

using ::testing::Test;
using namespace CoreD;

class DeviceItemGroupManagerTests : public Test {};

TEST_F( DeviceItemGroupManagerTests, matchDeviceGroupsForItemUpdatedEvent ) {
    Json eventData1 = R"({
        "_id": "item_id_1",
        "deviceArmed": true,
        "deviceCategory": "common_category",
        "deviceId": "device_id_1",
        "deviceName": "device_name_1",
        "deviceSubcategory": "subcategory_1",
        "fwTimestampMs": 1641461061891,
        "name": "item_name_1",
        "notifications": null,
        "parentDeviceId": "parent_device_id_1",
        "roomName": "",
        "serviceNotification": false,
        "syncNotification": false,
        "userNotification": false,
        "value": "item_value_1",
        "valueFormatted": "true",
        "valueType": "token"
    })"_json;

    Json eventData2 = R"({
        "_id": "item_id_2",
        "deviceArmed": true,
        "deviceCategory": "common_category",
        "deviceId": "device_id_2",
        "deviceName": "device_name_2",
        "fwTimestampMs": 1641461061891,
        "name": "item_name_2",
        "notifications": null,
        "parentDeviceId": "parent_device_id",
        "roomName": "",
        "serviceNotification": false,
        "syncNotification": false,
        "userNotification": false,
        "value": true,
        "valueFormatted": "true",
        "valueType": "bool"
    })"_json;

    const std::set< std::string > matchResult{ "group_id" };

    {  // Test exceptions
        DeviceItemGroupManager manager;
        Json deviceGroup = JSON_ARRAY( JSON( "_id"
                                             << "group_id"
                                             << "exceptions" << JSON_ARRAY( eventData1[ "deviceId" ] ) ) );
        manager.setDeviceGroups( deviceGroup );

        EXPECT_TRUE( manager.matchDeviceGroupsForItemUpdatedEvent( eventData1 ).empty() ) << "exceptions for event1";
        EXPECT_EQ( manager.matchDeviceGroupsForItemUpdatedEvent( eventData2 ), matchResult );
    }

    {  // Test devices without categories
        DeviceItemGroupManager manager;
        Json deviceGroup = JSON_ARRAY( JSON( "_id"
                                             << "group_id"
                                             << "devices" << JSON_ARRAY( eventData1[ "deviceId" ] ) ) );
        manager.setDeviceGroups( deviceGroup );

        EXPECT_EQ( manager.matchDeviceGroupsForItemUpdatedEvent( eventData1 ), matchResult );
        EXPECT_TRUE( manager.matchDeviceGroupsForItemUpdatedEvent( eventData2 ).empty() );
    }

    {  // Test category only
        DeviceItemGroupManager manager;
        Json deviceGroup = JSON_ARRAY( JSON( "_id"
                                             << "group_id"
                                             << "devices" << JSON_ARRAY( eventData1[ "deviceId" ] ) << "categories"
                                             << JSON_ARRAY( JSON( "category"
                                                                  << "common_category" ) ) ) );
        manager.setDeviceGroups( deviceGroup );

        EXPECT_EQ( manager.matchDeviceGroupsForItemUpdatedEvent( eventData1 ), matchResult );
        EXPECT_EQ( manager.matchDeviceGroupsForItemUpdatedEvent( eventData2 ), matchResult );
    }

    {  // Test subcategories only
        DeviceItemGroupManager manager;
        Json deviceGroup = JSON_ARRAY( JSON( "_id"
                                             << "group_id"
                                             << "categories"
                                             << JSON_ARRAY(
                                                    JSON( "subcategories" << JSON_ARRAY( "subcategory_1" ) ) ) ) );
        manager.setDeviceGroups( deviceGroup );

        EXPECT_EQ( manager.matchDeviceGroupsForItemUpdatedEvent( eventData1 ), matchResult );
        EXPECT_TRUE( manager.matchDeviceGroupsForItemUpdatedEvent( eventData2 ).empty() );
    }

    {  // Test subcategory exceptions only
        DeviceItemGroupManager manager;
        Json deviceGroup = JSON_ARRAY( JSON( "_id"
                                             << "group_id"
                                             << "categories"
                                             << JSON_ARRAY( JSON( "exceptions" << JSON_ARRAY( "subcategory_1" ) ) ) ) );
        manager.setDeviceGroups( deviceGroup );

        EXPECT_TRUE( manager.matchDeviceGroupsForItemUpdatedEvent( eventData1 ).empty() );
        EXPECT_EQ( manager.matchDeviceGroupsForItemUpdatedEvent( eventData2 ), matchResult );
    }

    {  // Test category + subcategories
        DeviceItemGroupManager manager;
        Json deviceGroup = JSON_ARRAY( JSON( "_id"
                                             << "group_id"
                                             << "categories"
                                             << JSON_ARRAY( JSON( "category"
                                                                  << "common_category"
                                                                  << "subcategories"
                                                                  << JSON_ARRAY( "subcategory_1" ) ) ) ) );
        manager.setDeviceGroups( deviceGroup );

        EXPECT_EQ( manager.matchDeviceGroupsForItemUpdatedEvent( eventData1 ), matchResult );
        EXPECT_TRUE( manager.matchDeviceGroupsForItemUpdatedEvent( eventData2 ).empty() );
    }

    {  // Test category + subcategory exceptions
        DeviceItemGroupManager manager;
        Json deviceGroup = JSON_ARRAY( JSON( "_id"
                                             << "group_id"
                                             << "categories"
                                             << JSON_ARRAY( JSON( "category"
                                                                  << "common_category"
                                                                  << "exceptions"
                                                                  << JSON_ARRAY( "subcategory_1" ) ) ) ) );
        manager.setDeviceGroups( deviceGroup );

        EXPECT_TRUE( manager.matchDeviceGroupsForItemUpdatedEvent( eventData1 ).empty() );
        EXPECT_EQ( manager.matchDeviceGroupsForItemUpdatedEvent( eventData2 ), matchResult );
    }

    {  // Test combined
        DeviceItemGroupManager manager;
        Json deviceGroup = JSON_ARRAY( JSON( "_id"
                                             << "group_id"
                                             << "devices" << JSON_ARRAY( eventData2[ "deviceId" ] ) << "categories"
                                             << JSON_ARRAY( JSON( "category"
                                                                  << "common_category"
                                                                  << "subcategories"
                                                                  << JSON_ARRAY( "subcategory_1" ) ) ) ) );
        manager.setDeviceGroups( deviceGroup );

        EXPECT_EQ( manager.matchDeviceGroupsForItemUpdatedEvent( eventData1 ), matchResult );
        EXPECT_EQ( manager.matchDeviceGroupsForItemUpdatedEvent( eventData2 ), matchResult );
    }

    {  // Test not match device AND not match category
        DeviceItemGroupManager manager;
        Json deviceGroup = JSON_ARRAY( JSON( "_id"
                                             << "group_id"
                                             << "devices" << JSON_ARRAY( eventData1[ "deviceId" ] ) << "categories"
                                             << JSON_ARRAY( JSON( "category"
                                                                  << "unknown" ) ) ) );
        manager.setDeviceGroups( deviceGroup );
        EXPECT_TRUE( manager.matchDeviceGroupsForItemUpdatedEvent( eventData2 ).empty() );
    }
}

TEST_F( DeviceItemGroupManagerTests, matchItemGroupsForItemUpdatedEvent ) {
    Json eventData1 = R"({
        "_id": "item_id_1",
        "deviceArmed": true,
        "deviceCategory": "common_category",
        "deviceId": "device_id_1",
        "deviceName": "device_name_1",
        "deviceSubcategory": "subcategory_1",
        "fwTimestampMs": 1641461061891,
        "name": "item_name_1",
        "notifications": null,
        "parentDeviceId": "parent_device_id_1",
        "roomName": "",
        "serviceNotification": false,
        "syncNotification": false,
        "userNotification": false,
        "value": "item_value_1",
        "valueFormatted": "true",
        "valueType": "token"
    })"_json;

    Json eventData2 = R"({
        "_id": "item_id_2",
        "deviceArmed": true,
        "deviceCategory": "common_category",
        "deviceId": "device_id_2",
        "deviceName": "device_name_2",
        "fwTimestampMs": 1641461061891,
        "name": "item_name_2",
        "notifications": null,
        "parentDeviceId": "parent_device_id",
        "roomName": "",
        "serviceNotification": false,
        "syncNotification": false,
        "userNotification": false,
        "value": true,
        "valueFormatted": "true",
        "valueType": "bool"
    })"_json;

    const std::set< std::string > matchResult{ "group_id" };

    {  // Test exceptions
        DeviceItemGroupManager manager;
        Json deviceGroup = JSON_ARRAY( JSON( "_id"
                                             << "group_id"
                                             << "exceptions" << JSON_ARRAY( eventData1[ "_id" ] ) ) );
        manager.setItemGroups( deviceGroup );

        EXPECT_TRUE( manager.matchItemGroupsForItemUpdatedEvent( eventData1 ).empty() ) << "exceptions for event1";
        EXPECT_EQ( manager.matchItemGroupsForItemUpdatedEvent( eventData2 ), matchResult );
    }

    {  // Test items
        DeviceItemGroupManager manager;
        Json deviceGroup = JSON_ARRAY( JSON( "_id"
                                             << "group_id"
                                             << "items" << JSON_ARRAY( eventData1[ "_id" ] ) ) );
        manager.setItemGroups( deviceGroup );

        EXPECT_EQ( manager.matchItemGroupsForItemUpdatedEvent( eventData1 ), matchResult );
        EXPECT_TRUE( manager.matchItemGroupsForItemUpdatedEvent( eventData2 ).empty() );
    }

    {  // Test itemNames
        DeviceItemGroupManager manager;
        Json deviceGroup = JSON_ARRAY( JSON( "_id"
                                             << "group_id"
                                             << "itemNames" << JSON_ARRAY( "item_name_1" ) ) );
        manager.setItemGroups( deviceGroup );

        EXPECT_EQ( manager.matchItemGroupsForItemUpdatedEvent( eventData1 ), matchResult );
        EXPECT_TRUE( manager.matchItemGroupsForItemUpdatedEvent( eventData2 ).empty() );
    }

    {  // Test valueType
        DeviceItemGroupManager manager;
        Json deviceGroup = JSON_ARRAY( JSON( "_id"
                                             << "group_id"
                                             << "valueType"
                                             << "token" ) );
        manager.setItemGroups( deviceGroup );

        EXPECT_EQ( manager.matchItemGroupsForItemUpdatedEvent( eventData1 ), matchResult );
        EXPECT_TRUE( manager.matchItemGroupsForItemUpdatedEvent( eventData2 ).empty() );
    }

    {  // Test enums: known enum
        DeviceItemGroupManager manager;
        Json deviceGroup = JSON_ARRAY( JSON( "_id"
                                             << "group_id"
                                             << "enums" << JSON_ARRAY( "item_value_1" ) ) );
        manager.setItemGroups( deviceGroup );

        EXPECT_EQ( manager.matchItemGroupsForItemUpdatedEvent( eventData1 ), matchResult );
    }

    {  // Test enums: unrelated enum
        DeviceItemGroupManager manager;
        Json deviceGroup = JSON_ARRAY( JSON( "_id"
                                             << "group_id"
                                             << "enums" << JSON_ARRAY( "unrelated" ) ) );
        manager.setItemGroups( deviceGroup );

        EXPECT_TRUE( manager.matchItemGroupsForItemUpdatedEvent( eventData1 ).empty() );
    }

    {  // Test unknown item AND unknown name
        DeviceItemGroupManager manager;
        Json deviceGroup = JSON_ARRAY( JSON( "_id"
                                             << "group_id"
                                             << "items" << JSON_ARRAY( "unknown_id" ) << "itemNames"
                                             << JSON_ARRAY( "unknown_name" ) ) );
        manager.setItemGroups( deviceGroup );

        EXPECT_TRUE( manager.matchItemGroupsForItemUpdatedEvent( eventData1 ).empty() );
    }

    {  // Test mismatched item id AND matched name + matched type
        DeviceItemGroupManager manager;
        Json deviceGroup = JSON_ARRAY( JSON( "_id"
                                             << "group_id"
                                             << "items" << JSON_ARRAY( "unknown_id" ) << "itemNames"
                                             << JSON_ARRAY( "item_name_1" ) << "valueType"
                                             << "token" ) );
        manager.setItemGroups( deviceGroup );

        EXPECT_EQ( manager.matchItemGroupsForItemUpdatedEvent( eventData1 ), matchResult );
    }

    {  // Test mismatched item id AND matched name BUT mismatched type
        DeviceItemGroupManager manager;
        Json deviceGroup = JSON_ARRAY( JSON( "_id"
                                             << "group_id"
                                             << "items" << JSON_ARRAY( "unknown_id" ) << "itemNames"
                                             << JSON_ARRAY( "item_name_1" ) << "valueType"
                                             << "unknown_type" ) );
        manager.setItemGroups( deviceGroup );

        EXPECT_TRUE( manager.matchItemGroupsForItemUpdatedEvent( eventData1 ).empty() );
    }
}

}  // namespace UT
}  // namespace HubPlatform
