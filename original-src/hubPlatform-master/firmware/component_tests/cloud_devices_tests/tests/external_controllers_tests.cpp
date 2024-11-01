#include "cloud_devices_tests/tests/fixtures/external_controllers_fixture.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <chrono>

namespace HubPlatform {
namespace Tests {

/*

TESTPLAN

DONE    1. Subscriptions
DONE        1.1 Subscribe/Unsubscribe items
DONE        1.2 Subscribe/Unsubscribe devices
TODO        1.3 Add additional items subscriptions for same controller
TODO        1.4 Add additional devices subscriptions for same controller
TODO        1.5 Check subscriptions on startup
TODO        1.6 Check subscriptions after controller reconnect
TODO        1.7 Completely unsubscribe from controller
TODO        1.8 Maintain separate requests queue for each controller
DONE    2 Handle items events
DONE        2.1 Item updated
DONE        2.2 Item dictionary updated
DONE            2.2.1 Added
DONE            2.2.2 Updated
DONE            2.2.3 Removed
DONE        2.3 Item removed
DONE        2.4 Out-of-date broadcasts
DONE        2.5 Ignore unsubscribed
DONE        2.6 Item not changed
DONE    3 Handle devices events
DONE        3.1 Device updated
DONE        3.2 Device removed
DONE        3.3 Out-of-date broadcasts
DONE        3.4 Ignore unsubscribed
DONE        3.5 Device not changed
DONE    4 Actions
DONE        4.1 Set item value
DONE        4.2 Set device armed
TODO    5. Handle request errors
TODO        5.1 Items list
TODO        5.2 Devices list
TODO        5.3 Set item value
TODO        5.4 Set device armed
TODO        5.5 Unsupported
TODO        5.6 Reask scheduled before error received
DONE    6. Hub bridge controller connections
DONE        6.1 Add
DONE        6.2 Update
DONE        6.3 Remove
DONE        6.4 With credentials
DONE        6.5 Only credentials

 */

TEST_F( ExternalControllersTest, SubscribeUnsubscribeItems ) {  // 1.1
    setCredentials( 110, "&0000110", "&0000110-uuid", { "&0000110-key-1", "&0000110-key-2" }, FwCertsType::Broken );

    // Subscribe on item changes
    externalControllers().subscribeOnItemChanges( "&0000110", { "item-1.1.1", "item-1.1.3" } );

    EXPECT_EQ( "{}", externalControllers().getItem( "&0000110", "item-1.1.1" ).dump( 4 ) );
    EXPECT_EQ( "{}", externalControllers().getItem( "&0000110", "item-1.1.3" ).dump( 4 ) );
    EXPECT_TRUE( externalControllers().getItems( "&0000110", { "item-1.1.1", "item-1.1.3" } ).empty() );

    {
        auto request = TEST_WAIT_FOR_REQUEST();

        EXPECT_EQ( json_get_str( request, "method" ), "cloud.controller_raw_command" );
        EXPECT_EQ( json_get_str( request, "api" ), "1" );
        EXPECT_EQ( json_get_int( request[ "params" ], "instant" ), 1 );
        EXPECT_EQ( json_get_int( request[ "params" ], "queued" ), 0 );
        EXPECT_EQ( json_get_str( request[ "params" ], "controller_uuid" ), "&0000110-uuid" );
        EXPECT_EQ( json_get_str( request[ "params" ][ "command" ], "method" ), "hub.items.list" );
        auto internalId = json_get_str( request[ "params" ][ "command" ], "id" );
        auto items = generateItems( "", { "item-1.1.1", "item-1.1.2", "item-1.1.3" } );

        sendResponse( request,
                      makeRawCommandResult( internalId, "hub.items.list", JSON( "items" << items ) ),
                      JSON_EMPTY_OBJ,
                      JSON( "type"
                            << "controller"
                            << "uuid"
                            << "&0000110-uuid" ) );
    }

    {
        auto broadcast = NMA_TEST_WAIT_FOR_INTERNAL_BROADCAST();
        EXPECT_EQ( broadcast.msgSubClass, HUB_ITEM_ADDED );
        EXPECT_EQ( generateItem( "&0000110", "item-1.1.1" ), broadcast.data.dump( 4 ) );
    }
    {
        auto broadcast = NMA_TEST_WAIT_FOR_INTERNAL_BROADCAST();
        EXPECT_EQ( broadcast.msgSubClass, HUB_ITEM_ADDED );
        EXPECT_EQ( generateItem( "&0000110", "item-1.1.3" ), broadcast.data.dump( 4 ) );
    }

    EXPECT_EQ( generateItem( "&0000110", "item-1.1.1" ),
               externalControllers().getItem( "&0000110", "item-1.1.1" ).dump( 4 ) );
    EXPECT_EQ( generateItem( "&0000110", "item-1.1.3" ),
               externalControllers().getItem( "&0000110", "item-1.1.3" ).dump( 4 ) );
    ASSERT_THAT(
        dump( externalControllers().getItems( "&0000110", { "item-1.1.1", "item-1.1.3" } ) ),
        ::testing::ElementsAre( generateItem( "&0000110", "item-1.1.1" ), generateItem( "&0000110", "item-1.1.3" ) ) );

    // Unsubscribe from item changes
    externalControllers().unsubscribeFromItemChanges( "&0000110", { "item-1.1.1" } );

    std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );

    EXPECT_EQ( "{}", externalControllers().getItem( "&0000110", "item-1.1.1" ).dump( 4 ) );
    EXPECT_EQ( generateItem( "&0000110", "item-1.1.3" ),
               externalControllers().getItem( "&0000110", "item-1.1.3" ).dump( 4 ) );
    ASSERT_THAT( dump( externalControllers().getItems( "&0000110", { "item-1.1.1", "item-1.1.3" } ) ),
                 ::testing::ElementsAre( generateItem( "&0000110", "item-1.1.3" ) ) );

    // Teardown
    externalControllers().unsubscribeFromItemChanges( "&0000110", { "item-1.1.3" } );
}

TEST_F( ExternalControllersTest, SubscribeUnsubscribeDevices ) {  // 1.2
    setCredentials( 120, "&0000120", "&0000120-uuid", { "&0000120-key-1", "&0000120-key-2" }, FwCertsType::Broken );

    // Subscribe on devices changes
    externalControllers().subscribeOnDevicesChanges( "&0000120", { "device-1.1", "device-1.3" } );

    EXPECT_EQ( "{}", externalControllers().getSingleDevice( "&0000120", "device-1.1" ).dump( 4 ) );
    EXPECT_EQ( "{}", externalControllers().getSingleDevice( "&0000120", "device-1.3" ).dump( 4 ) );

    {
        auto request = TEST_WAIT_FOR_REQUEST();

        EXPECT_EQ( json_get_str( request, "method" ), "cloud.controller_raw_command" );
        EXPECT_EQ( json_get_str( request, "api" ), "1" );
        EXPECT_EQ( json_get_int( request[ "params" ], "instant" ), 1 );
        EXPECT_EQ( json_get_int( request[ "params" ], "queued" ), 0 );
        EXPECT_EQ( json_get_str( request[ "params" ], "controller_uuid" ), "&0000120-uuid" );
        EXPECT_EQ( json_get_str( request[ "params" ][ "command" ], "method" ), "hub.devices.list" );
        auto internalId = json_get_str( request[ "params" ][ "command" ], "id" );
        auto devices = generateDevices( "", { "device-1.1", "device-1.2", "device-1.3" } );
        sendResponse( request,
                      makeRawCommandResult( internalId, "hub.devices.list", JSON( "devices" << devices ) ),
                      JSON_EMPTY_OBJ,
                      JSON( "type"
                            << "controller"
                            << "uuid"
                            << "&0000120-uuid" ) );
    }

    {
        auto broadcast = NMA_TEST_WAIT_FOR_INTERNAL_BROADCAST();
        EXPECT_EQ( broadcast.msgSubClass, HUB_DEVICE_ADDED );
        EXPECT_EQ( generateDevice( "&0000120", "device-1.1" ), broadcast.data.dump( 4 ) );
    }
    {
        auto broadcast = NMA_TEST_WAIT_FOR_INTERNAL_BROADCAST();
        EXPECT_EQ( broadcast.msgSubClass, HUB_DEVICE_ADDED );
        EXPECT_EQ( generateDevice( "&0000120", "device-1.3" ), broadcast.data.dump( 4 ) );
    }

    EXPECT_EQ( generateDevice( "&0000120", "device-1.1" ),
               externalControllers().getSingleDevice( "&0000120", "device-1.1" ).dump( 4 ) );
    EXPECT_EQ( generateDevice( "&0000120", "device-1.3" ),
               externalControllers().getSingleDevice( "&0000120", "device-1.3" ).dump( 4 ) );

    // Unsubscribe from devices changes
    externalControllers().unsubscribeFromDevicesChanges( "&0000120", { "device-1.1" } );

    std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );

    EXPECT_EQ( "{}", externalControllers().getSingleDevice( "&0000120", "device-1.1" ).dump( 4 ) );
    EXPECT_EQ( generateDevice( "&0000120", "device-1.3" ),
               externalControllers().getSingleDevice( "&0000120", "device-1.3" ).dump( 4 ) );

    // Teardown
    externalControllers().unsubscribeFromDevicesChanges( "&0000110", { "device-1.3" } );
}

TEST_F( ExternalControllersTest, handleItemUpdatedBroadcast ) {  // 2.1

    setCredentials( 210, "&0000210", "&0000210-uuid", { "&0000210-key-1", "&0000210-key-2" }, FwCertsType::Broken );
    subscribeOnItemChanges( "&0000210", { "item-1.1.1", "item-1.1.3" } );

    sendBroadcast( "hub.item.updated",
                   PARSE_JSON( "{\n"
                               "    \"_id\": \"item-1.1.3\",\n"
                               "    \"deviceCategory\": \"dimmable_light\",\n"
                               "    \"deviceId\": \"device-1.1\",\n"
                               "    \"deviceName\": \"Dimmer\",\n"
                               "    \"deviceSubcategory\": \"dimmable_plugged\",\n"
                               "    \"name\": \"switch\",\n"
                               "    \"notifications\": null,\n"
                               "    \"roomName\": \"\",\n"
                               "    \"serviceNotification\": false,\n"
                               "    \"syncNotification\": false,\n"
                               "    \"userNotification\": true,\n"
                               "    \"value\": true,\n"
                               "    \"valueFormatted\": \"true\",\n"
                               "    \"valueType\": \"bool\",\n"
                               "    \"nma_timestamp\": 1586762888663,\n"
                               "    \"controller_id\": \"&0000210\",\n"
                               "    \"controller_uuid\": \"&0000210-uuid\""
                               "}" ),
                   JSON( "type"
                         << "controller"
                         << "uuid"
                         << "&0000210-uuid" ) );

    {
        auto broadcast = NMA_TEST_WAIT_FOR_INTERNAL_BROADCAST();
        EXPECT_EQ( broadcast.msgSubClass, HUB_ITEM_UPDATED );
        EXPECT_EQ( "{\n"
                   "    \"_id\": \"item-1.1.3\",\n"
                   "    \"controllerId\": \"&0000210\",\n"
                   "    \"deviceId\": \"device-1.1\",\n"
                   "    \"name\": \"switch\",\n"
                   "    \"value\": true,\n"
                   "    \"valueFormatted\": \"true\",\n"
                   "    \"valueType\": \"bool\"\n"
                   "}",
                   broadcast.data.dump( 4 ) );
    }

    EXPECT_EQ( "{\n"
               "    \"_id\": \"item-1.1.3\",\n"
               "    \"controllerId\": \"&0000210\",\n"
               "    \"deviceId\": \"device-1.1\",\n"
               "    \"hasGetter\": true,\n"
               "    \"hasSetter\": true,\n"
               "    \"name\": \"switch\",\n"
               "    \"show\": true,\n"
               "    \"value\": true,\n"
               "    \"valueFormatted\": \"true\",\n"
               "    \"valueType\": \"bool\"\n"
               "}",
               externalControllers().getItem( "&0000210", "item-1.1.3" ).dump( 4 ) );

    // Teardown
    externalControllers().unsubscribeFromItemChanges( "&0000210", { "item-1.1.1", "item-1.1.3" } );
}

TEST_F( ExternalControllersTest, handleItemDictionaryAddedBroadcast ) {  // 2.2.1

    setCredentials( 221, "&0000221", "&0000221-uuid", { "&0000221-key-1", "&0000221-key-2" }, FwCertsType::Broken );
    subscribeOnItemChanges( "&0000221", { "item-1.1.1", "item-1.1.3" } );

    sendBroadcast( "hub.item.dictionary.updated",
                   PARSE_JSON( "{\n"
                               "    \"_id\": \"item-1.1.1\",\n"
                               "    \"deviceCategory\": \"door_lock\",\n"
                               "    \"deviceId\": \"device-1.1\",\n"
                               "    \"deviceName\": \"Touchscreen Deadbolt\",\n"
                               "    \"deviceSubcategory\": \"\",\n"
                               "    \"name\": \"user_codes\",\n"
                               "    \"notifications\": null,\n"
                               "    \"roomName\": \"\",\n"
                               "    \"serviceNotification\": false,\n"
                               "    \"syncNotification\": false,\n"
                               "    \"userNotification\": true,\n"
                               "    \"operation\": \"added\",\n"
                               "    \"element\": {\n"
                               "        \"3\": {\n"
                               "            \"value\": {\n"
                               "                \"code\": \"3333\",\n"
                               "                \"name\": \"test3\"\n"
                               "            }\n"
                               "        }\n"
                               "    }\n"
                               "}" ),
                   JSON( "type"
                         << "controller"
                         << "uuid"
                         << "&0000221-uuid" ) );

    {
        auto broadcast = NMA_TEST_WAIT_FOR_INTERNAL_BROADCAST();
        EXPECT_EQ( broadcast.msgSubClass, HUB_ITEM_DICTIONARY_UPDATED );
        EXPECT_EQ( "{\n"
                   "    \"_id\": \"item-1.1.1\",\n"
                   "    \"controllerId\": \"&0000221\",\n"
                   "    \"deviceId\": \"device-1.1\",\n"
                   "    \"element\": {\n"
                   "        \"3\": {\n"
                   "            \"value\": {\n"
                   "                \"code\": \"3333\",\n"
                   "                \"name\": \"test3\"\n"
                   "            }\n"
                   "        }\n"
                   "    },\n"
                   "    \"name\": \"user_codes\",\n"
                   "    \"operation\": \"added\"\n"
                   "}",
                   broadcast.data.dump( 4 ) );
    }

    EXPECT_EQ( "{\n"
               "    \"_id\": \"item-1.1.1\",\n"
               "    \"controllerId\": \"&0000221\",\n"
               "    \"deviceId\": \"device-1.1\",\n"
               "    \"elementType\": \"userCode\",\n"
               "    \"elementsMaxNumber\": 100,\n"
               "    \"hasGetter\": true,\n"
               "    \"hasSetter\": true,\n"
               "    \"name\": \"user_codes\",\n"
               "    \"show\": true,\n"
               "    \"userCodeRestriction\": \"^\\\\d{4}$\",\n"
               "    \"value\": {\n"
               "        \"1\": {\n"
               "            \"code\": \"1111\",\n"
               "            \"name\": \"test1\"\n"
               "        },\n"
               "        \"2\": {\n"
               "            \"code\": \"2222\",\n"
               "            \"name\": \"test2\"\n"
               "        },\n"
               "        \"3\": {\n"
               "            \"code\": \"3333\",\n"
               "            \"name\": \"test3\"\n"
               "        }\n"
               "    },\n"
               "    \"valueFormatted\": \"\",\n"
               "    \"valueType\": \"dictionary\"\n"
               "}",
               externalControllers().getItem( "&0000221", "item-1.1.1" ).dump( 4 ) );

    // Teardown
    externalControllers().unsubscribeFromItemChanges( "&0000221", { "item-1.1.1", "item-1.1.3" } );
}

TEST_F( ExternalControllersTest, handleItemDictionaryUpdatedBroadcast ) {  // 2.2.2

    setCredentials( 222, "&0000222", "&0000222-uuid", { "&0000222-key-1", "&0000222-key-2" }, FwCertsType::Broken );
    subscribeOnItemChanges( "&0000222", { "item-1.1.1", "item-1.1.3" } );

    sendBroadcast( "hub.item.dictionary.updated",
                   PARSE_JSON( "{\n"
                               "    \"_id\": \"item-1.1.1\",\n"
                               "    \"deviceCategory\": \"door_lock\",\n"
                               "    \"deviceId\": \"device-1.1\",\n"
                               "    \"deviceName\": \"Touchscreen Deadbolt\",\n"
                               "    \"deviceSubcategory\": \"\",\n"
                               "    \"name\": \"user_codes\",\n"
                               "    \"notifications\": null,\n"
                               "    \"roomName\": \"\",\n"
                               "    \"serviceNotification\": false,\n"
                               "    \"syncNotification\": false,\n"
                               "    \"userNotification\": true,\n"
                               "    \"operation\": \"updated\",\n"
                               "    \"element\": {\n"
                               "        \"2\": {\n"
                               "            \"value\": {\n"
                               "                \"code\": \"2233\",\n"
                               "                \"name\": \"new2\"\n"
                               "            }\n"
                               "        }\n"
                               "    }\n"
                               "}" ),
                   JSON( "type"
                         << "controller"
                         << "uuid"
                         << "&0000222-uuid" ) );

    {
        auto broadcast = NMA_TEST_WAIT_FOR_INTERNAL_BROADCAST();
        EXPECT_EQ( broadcast.msgSubClass, HUB_ITEM_DICTIONARY_UPDATED );
        EXPECT_EQ( "{\n"
                   "    \"_id\": \"item-1.1.1\",\n"
                   "    \"controllerId\": \"&0000222\",\n"
                   "    \"deviceId\": \"device-1.1\",\n"
                   "    \"element\": {\n"
                   "        \"2\": {\n"
                   "            \"value\": {\n"
                   "                \"code\": \"2233\",\n"
                   "                \"name\": \"new2\"\n"
                   "            }\n"
                   "        }\n"
                   "    },\n"
                   "    \"name\": \"user_codes\",\n"
                   "    \"operation\": \"updated\"\n"
                   "}",
                   broadcast.data.dump( 4 ) );
    }

    EXPECT_EQ( "{\n"
               "    \"_id\": \"item-1.1.1\",\n"
               "    \"controllerId\": \"&0000222\",\n"
               "    \"deviceId\": \"device-1.1\",\n"
               "    \"elementType\": \"userCode\",\n"
               "    \"elementsMaxNumber\": 100,\n"
               "    \"hasGetter\": true,\n"
               "    \"hasSetter\": true,\n"
               "    \"name\": \"user_codes\",\n"
               "    \"show\": true,\n"
               "    \"userCodeRestriction\": \"^\\\\d{4}$\",\n"
               "    \"value\": {\n"
               "        \"1\": {\n"
               "            \"code\": \"1111\",\n"
               "            \"name\": \"test1\"\n"
               "        },\n"
               "        \"2\": {\n"
               "            \"code\": \"2233\",\n"
               "            \"name\": \"new2\"\n"
               "        }\n"
               "    },\n"
               "    \"valueFormatted\": \"\",\n"
               "    \"valueType\": \"dictionary\"\n"
               "}",
               externalControllers().getItem( "&0000222", "item-1.1.1" ).dump( 4 ) );

    // Teardown
    externalControllers().unsubscribeFromItemChanges( "&0000222", { "item-1.1.1", "item-1.1.3" } );
}

TEST_F( ExternalControllersTest, handleItemDictionaryRemovedBroadcast ) {  // 2.2.3

    setCredentials( 223, "&0000223", "&0000223-uuid", { "&0000223-key-1", "&0000223-key-2" }, FwCertsType::Broken );
    subscribeOnItemChanges( "&0000223", { "item-1.1.1", "item-1.1.3" } );

    sendBroadcast( "hub.item.dictionary.updated",
                   PARSE_JSON( "{\n"
                               "    \"_id\": \"item-1.1.1\",\n"
                               "    \"deviceCategory\": \"door_lock\",\n"
                               "    \"deviceId\": \"device-1.1\",\n"
                               "    \"deviceName\": \"Touchscreen Deadbolt\",\n"
                               "    \"deviceSubcategory\": \"\",\n"
                               "    \"name\": \"user_codes\",\n"
                               "    \"notifications\": null,\n"
                               "    \"roomName\": \"\",\n"
                               "    \"serviceNotification\": false,\n"
                               "    \"syncNotification\": false,\n"
                               "    \"userNotification\": true,\n"
                               "    \"operation\": \"removed\",\n"
                               "    \"element\": {\n"
                               "        \"2\": {\n"
                               "            \"value\": {\n"
                               "                \"code\": \"2222\",\n"
                               "                \"name\": \"test2\"\n"
                               "            }\n"
                               "        }\n"
                               "    }\n"
                               "}" ),
                   JSON( "type"
                         << "controller"
                         << "uuid"
                         << "&0000223-uuid" ) );

    {
        auto broadcast = NMA_TEST_WAIT_FOR_INTERNAL_BROADCAST();
        EXPECT_EQ( broadcast.msgSubClass, HUB_ITEM_DICTIONARY_UPDATED );
        EXPECT_EQ( "{\n"
                   "    \"_id\": \"item-1.1.1\",\n"
                   "    \"controllerId\": \"&0000223\",\n"
                   "    \"deviceId\": \"device-1.1\",\n"
                   "    \"element\": {\n"
                   "        \"2\": {\n"
                   "            \"value\": {\n"
                   "                \"code\": \"2222\",\n"
                   "                \"name\": \"test2\"\n"
                   "            }\n"
                   "        }\n"
                   "    },\n"
                   "    \"name\": \"user_codes\",\n"
                   "    \"operation\": \"removed\"\n"
                   "}",
                   broadcast.data.dump( 4 ) );
    }

    EXPECT_EQ( "{\n"
               "    \"_id\": \"item-1.1.1\",\n"
               "    \"controllerId\": \"&0000223\",\n"
               "    \"deviceId\": \"device-1.1\",\n"
               "    \"elementType\": \"userCode\",\n"
               "    \"elementsMaxNumber\": 100,\n"
               "    \"hasGetter\": true,\n"
               "    \"hasSetter\": true,\n"
               "    \"name\": \"user_codes\",\n"
               "    \"show\": true,\n"
               "    \"userCodeRestriction\": \"^\\\\d{4}$\",\n"
               "    \"value\": {\n"
               "        \"1\": {\n"
               "            \"code\": \"1111\",\n"
               "            \"name\": \"test1\"\n"
               "        }\n"
               "    },\n"
               "    \"valueFormatted\": \"\",\n"
               "    \"valueType\": \"dictionary\"\n"
               "}",
               externalControllers().getItem( "&0000223", "item-1.1.1" ).dump( 4 ) );

    // Teardown
    externalControllers().unsubscribeFromItemChanges( "&0000223", { "item-1.1.1", "item-1.1.3" } );
}

TEST_F( ExternalControllersTest, handleItemRemovedBroadcast ) {  // 2.3

    setCredentials( 230, "&0000230", "&0000230-uuid", { "&0000230-key-1", "&0000230-key-2" }, FwCertsType::Broken );
    subscribeOnItemChanges( "&0000230", { "item-1.1.1", "item-1.1.3" } );

    sendBroadcast( "hub.item.removed",
                   JSON( "_id"
                         << "item-1.1.3" ),
                   JSON( "type"
                         << "controller"
                         << "uuid"
                         << "&0000230-uuid" ) );

    {
        auto broadcast = NMA_TEST_WAIT_FOR_INTERNAL_BROADCAST();
        EXPECT_EQ( broadcast.msgSubClass, HUB_ITEM_REMOVED );
        EXPECT_EQ( "{\n"
                   "    \"_id\": \"item-1.1.3\",\n"
                   "    \"controllerId\": \"&0000230\"\n"
                   "}",
                   broadcast.data.dump( 4 ) );
    }

    EXPECT_EQ( "{}", externalControllers().getItem( "&0000230", "item-1.1.3" ).dump( 4 ) );
    EXPECT_EQ( generateItem( "&0000230", "item-1.1.1" ),
               externalControllers().getItem( "&0000230", "item-1.1.1" ).dump( 4 ) );
    ASSERT_THAT( dump( externalControllers().getItems( "&0000230", { "item-1.1.1", "item-1.1.3" } ) ),
                 ::testing::ElementsAre( generateItem( "&0000230", "item-1.1.1" ) ) );

    // Teardown
    externalControllers().unsubscribeFromItemChanges( "&0000230", { "item-1.1.1", "item-1.1.3" } );
}

TEST_F( ExternalControllersTest, handleOutOfDateItemBroadcast ) {  // 2.4

    // Setup
    setCredentials( 240, "&0000240", "&0000240-uuid", { "&0000240-key-1", "&0000240-key-2" }, FwCertsType::Broken );

    externalControllers().subscribeOnItemChanges( "&0000240", { "item-1.1.4" } );

    auto handleItemsListRequest = [&]() {
        auto request = TEST_WAIT_FOR_REQUEST();

        EXPECT_EQ( request[ "method" ], "cloud.controller_raw_command" );
        auto internalId = json_get_str( request[ "params" ][ "command" ], "id" );
        auto items = generateItems( "", { "item-1.1.1" } );
        sendResponse( request,
                      makeRawCommandResult( internalId, "hub.items.list", JSON( "items" << items ) ),
                      JSON_EMPTY_OBJ,
                      JSON( "type"
                            << "controller"
                            << "uuid"
                            << "&0000240-uuid" ) );
    };

    handleItemsListRequest();

    // Test
    sendBroadcast( "hub.item.removed",
                   JSON( "_id"
                         << "item-1.1.4" ),
                   JSON( "type"
                         << "controller"
                         << "uuid"
                         << "&0000240-uuid" ) );

    handleItemsListRequest();

    // Teardown
    std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
    externalControllers().unsubscribeFromItemChanges( "&0000240", { "item-1.1.1", "item-1.1.3" } );
}

TEST_F( ExternalControllersTest, handleIgnoreUnsubscribedItemBroadcast ) {  // 2.5

    setCredentials( 250, "&0000250", "&0000250-uuid", { "&0000250-key-1", "&0000250-key-2" }, FwCertsType::Broken );
    subscribeOnItemChanges( "&0000250", { "item-1.1.1" } );

    sendBroadcast( "hub.item.updated",
                   PARSE_JSON( "{\n"
                               "    \"_id\": \"item-1.1.3\",\n"
                               "    \"deviceCategory\": \"dimmable_light\",\n"
                               "    \"deviceId\": \"device-1.1\",\n"
                               "    \"deviceName\": \"Dimmer\",\n"
                               "    \"deviceSubcategory\": \"dimmable_plugged\",\n"
                               "    \"name\": \"switch\",\n"
                               "    \"notifications\": null,\n"
                               "    \"roomName\": \"\",\n"
                               "    \"serviceNotification\": false,\n"
                               "    \"syncNotification\": false,\n"
                               "    \"userNotification\": true,\n"
                               "    \"value\": true,\n"
                               "    \"valueFormatted\": \"true\",\n"
                               "    \"valueType\": \"bool\",\n"
                               "    \"nma_timestamp\": 1586762888663,\n"
                               "    \"controller_id\": \"&0000250\",\n"
                               "    \"controller_uuid\": \"&0000250-uuid\""
                               "}" ),
                   JSON( "type"
                         << "controller"
                         << "uuid"
                         << "&0000250-uuid" ) );

    NMA_TEST_WAIT_NO_INTERNAL_BROADCASTS();
    TEST_WAIT_NO_REQUESTS();

    // Teardown
    externalControllers().unsubscribeFromItemChanges( "&0000250", { "item-1.1.1", "item-1.1.3" } );
}

TEST_F( ExternalControllersTest, handleItemNotChangedBroadcast ) {  // 2.6

    setCredentials( 260, "&0000260", "&0000260-uuid", { "&0000260-key-1", "&0000260-key-2" }, FwCertsType::Broken );
    subscribeOnItemChanges( "&0000260", { "item-1.1.1", "item-1.1.3" } );

    sendBroadcast( "hub.item.updated",
                   PARSE_JSON( "{\n"
                               "    \"_id\": \"item-1.1.3\",\n"
                               "    \"deviceCategory\": \"dimmable_light\",\n"
                               "    \"deviceId\": \"device-1.1\",\n"
                               "    \"deviceName\": \"Dimmer\",\n"
                               "    \"deviceSubcategory\": \"dimmable_plugged\",\n"
                               "    \"name\": \"switch\",\n"
                               "    \"notifications\": null,\n"
                               "    \"roomName\": \"\",\n"
                               "    \"serviceNotification\": false,\n"
                               "    \"syncNotification\": false,\n"
                               "    \"userNotification\": true,\n"
                               "    \"value\": false,\n"
                               "    \"valueFormatted\": \"false\",\n"
                               "    \"valueType\": \"bool\",\n"
                               "    \"nma_timestamp\": 1586762888663,\n"
                               "    \"controller_id\": \"&0000260\",\n"
                               "    \"controller_uuid\": \"&0000260-uuid\""
                               "}" ),
                   JSON( "type"
                         << "controller"
                         << "uuid"
                         << "&0000260-uuid" ) );

    NMA_TEST_WAIT_NO_INTERNAL_BROADCASTS();

    EXPECT_EQ( "{\n"
               "    \"_id\": \"item-1.1.3\",\n"
               "    \"controllerId\": \"&0000260\",\n"
               "    \"deviceId\": \"device-1.1\",\n"
               "    \"hasGetter\": true,\n"
               "    \"hasSetter\": true,\n"
               "    \"name\": \"switch\",\n"
               "    \"show\": true,\n"
               "    \"value\": false,\n"
               "    \"valueFormatted\": \"false\",\n"
               "    \"valueType\": \"bool\"\n"
               "}",
               externalControllers().getItem( "&0000260", "item-1.1.3" ).dump( 4 ) );

    // Teardown
    externalControllers().unsubscribeFromItemChanges( "&0000260", { "item-1.1.1", "item-1.1.3" } );
}

TEST_F( ExternalControllersTest, handleDeviceUpdatedBroadcast ) {  // 3.1

    setCredentials( 310, "&0000310", "&0000310-uuid", { "&0000310-key-1", "&0000310-key-2" }, FwCertsType::Broken );
    subscribeOnDeviceChanges( "&0000310", { "device-1.1", "device-1.3" } );

    sendBroadcast( "hub.device.updated",
                   PARSE_JSON( "{\n"
                               "    \"_id\": \"device-1.1\",\n"
                               "    \"roomId\": \"room-1\",\n"
                               "    \"serviceNotification\": false,\n"
                               "    \"syncNotification\": false\n"
                               "}" ),
                   JSON( "type"
                         << "controller"
                         << "uuid"
                         << "&0000310-uuid" ) );

    {
        auto broadcast = NMA_TEST_WAIT_FOR_INTERNAL_BROADCAST();
        EXPECT_EQ( broadcast.msgSubClass, HUB_DEVICE_UPDATED );
        EXPECT_EQ( "{\n"
                   "    \"_id\": \"device-1.1\",\n"
                   "    \"controllerId\": \"&0000310\",\n"
                   "    \"roomId\": \"room-1\"\n"
                   "}",
                   broadcast.data.dump( 4 ) );
    }

    EXPECT_EQ( "{\n"
               "    \"_id\": \"device-1.1\",\n"
               "    \"batteryPowered\": false,\n"
               "    \"category\": \"dimmable_light\",\n"
               "    \"controllerId\": \"&0000310\",\n"
               "    \"deviceTypeId\": \"134_3_25\",\n"
               "    \"gatewayId\": \"gateway-1\",\n"
               "    \"info\": {\n"
               "        \"manufacturer\": \"Aeotec\",\n"
               "        \"model\": \"DSC25\"\n"
               "    },\n"
               "    \"name\": \"Dimmer\",\n"
               "    \"parentDeviceId\": \"\",\n"
               "    \"reachable\": true,\n"
               "    \"ready\": true,\n"
               "    \"roomId\": \"room-1\",\n"
               "    \"security\": \"no\",\n"
               "    \"status\": \"idle\",\n"
               "    \"subcategory\": \"dimmable_plugged\",\n"
               "    \"type\": \"dimmer.outlet\"\n"
               "}",
               externalControllers().getSingleDevice( "&0000310", "device-1.1" ).dump( 4 ) );

    // Teardown
    externalControllers().unsubscribeFromDevicesChanges( "&0000310", { "device-1.1", "device-1.3" } );
}

TEST_F( ExternalControllersTest, handleDeviceRemovedBroadcast ) {  // 3.2

    setCredentials( 320, "&0000320", "&0000320-uuid", { "&0000320-key-1", "&0000320-key-2" }, FwCertsType::Broken );
    subscribeOnDeviceChanges( "&0000320", { "device-1.1", "device-1.3" } );

    sendBroadcast( "hub.device.removed",
                   JSON( "_id"
                         << "device-1.3" ),
                   JSON( "type"
                         << "controller"
                         << "uuid"
                         << "&0000320-uuid" ) );

    {
        auto broadcast = NMA_TEST_WAIT_FOR_INTERNAL_BROADCAST();
        EXPECT_EQ( broadcast.msgSubClass, HUB_DEVICE_REMOVED );
        EXPECT_EQ( "{\n"
                   "    \"_id\": \"device-1.3\",\n"
                   "    \"controllerId\": \"&0000320\"\n"
                   "}",
                   broadcast.data.dump( 4 ) );
    }

    EXPECT_EQ( "{}", externalControllers().getSingleDevice( "&0000320", "device-1.3" ).dump( 4 ) );
    EXPECT_EQ( generateDevice( "&0000320", "device-1.1" ),
               externalControllers().getSingleDevice( "&0000320", "device-1.1" ).dump( 4 ) );

    // Teardown
    externalControllers().unsubscribeFromDevicesChanges( "&0000320", { "device-1.1", "device-1.3" } );
}

TEST_F( ExternalControllersTest, handleDeviceOutOfDateBroadcast ) {  // 3.3

    // Setup
    setCredentials( 330, "&0000330", "&0000330-uuid", { "&0000330-key-1", "&0000330-key-2" }, FwCertsType::Broken );
    externalControllers().subscribeOnDevicesChanges( "&0000330", { "device-1.4" } );

    auto handleDevicesListRequest = [&]() {
        auto request = TEST_WAIT_FOR_REQUEST();

        EXPECT_EQ( request[ "method" ], "cloud.controller_raw_command" );
        auto internalId = json_get_str( request[ "params" ][ "command" ], "id" );
        auto devices = generateDevices( "", { "device-1.1" } );
        sendResponse( request,
                      makeRawCommandResult( internalId, "hub.devices.list", JSON( "devices" << devices ) ),
                      JSON_EMPTY_OBJ,
                      JSON( "type"
                            << "controller"
                            << "uuid"
                            << "&0000330-uuid" ) );
    };

    handleDevicesListRequest();

    // Test
    sendBroadcast( "hub.device.removed",
                   JSON( "_id"
                         << "device-1.4" ),
                   JSON( "type"
                         << "controller"
                         << "uuid"
                         << "&0000330-uuid" ) );

    handleDevicesListRequest();

    // Teardown
    std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
    externalControllers().unsubscribeFromDevicesChanges( "&0000330", { "device-1.1", "device-1.3" } );
}

TEST_F( ExternalControllersTest, handleUnsubscribedDeviceBroadcast ) {  // 3.4

    setCredentials( 340, "&0000340", "&0000340-uuid", { "&0000340-key-1", "&0000340-key-2" }, FwCertsType::Broken );
    subscribeOnDeviceChanges( "&0000340", { "device-1.3" } );

    sendBroadcast( "hub.device.updated",
                   PARSE_JSON( "{\n"
                               "    \"_id\": \"device-1.1\",\n"
                               "    \"roomId\": \"\",\n"
                               "    \"serviceNotification\": false,\n"
                               "    \"syncNotification\": false\n"
                               "}" ),
                   JSON( "type"
                         << "controller"
                         << "uuid"
                         << "&0000340-uuid" ) );

    NMA_TEST_WAIT_NO_INTERNAL_BROADCASTS();
    TEST_WAIT_NO_REQUESTS();

    // Teardown
    externalControllers().unsubscribeFromDevicesChanges( "&0000340", { "device-1.1", "device-1.3" } );
}

TEST_F( ExternalControllersTest, handleDeviceNotChangedBroadcast ) {  // 3.5

    setCredentials( 350, "&0000350", "&0000350-uuid", { "&0000350-key-1", "&0000350-key-2" }, FwCertsType::Broken );
    subscribeOnDeviceChanges( "&0000350", { "device-1.1", "device-1.3" } );

    sendBroadcast( "hub.device.updated",
                   PARSE_JSON( "{\n"
                               "    \"_id\": \"device-1.1\",\n"
                               "    \"roomId\": \"\",\n"
                               "    \"serviceNotification\": false,\n"
                               "    \"syncNotification\": false\n"
                               "}" ),
                   JSON( "type"
                         << "controller"
                         << "uuid"
                         << "&0000350-uuid" ) );

    NMA_TEST_WAIT_NO_INTERNAL_BROADCASTS();

    // Teardown
    externalControllers().unsubscribeFromDevicesChanges( "&0000350", { "device-1.1", "device-1.3" } );
}

TEST_F( ExternalControllersTest, setItemValue ) {  // 4.1

    std::mutex responseMutex;
    std::condition_variable responseCondVar;
    Json actualResponse;

    setCredentials( 410, "&0000410", "&0000410-uuid", { "&0000410-key-1", "&0000410-key-2" }, FwCertsType::Broken );
    std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
    externalControllers().setItemValue( "&0000410",
                                        JSON( "_id"
                                              << "item-1.1.2"
                                              << "value" << 30 ),
                                        [&]( const JsonObject& response ) {
                                            actualResponse = response[ "result" ];
                                            responseCondVar.notify_one();
                                        } );

    {
        auto request = TEST_WAIT_FOR_REQUEST();

        EXPECT_EQ( json_get_str( request, "method" ), "cloud.controller_raw_command" );
        EXPECT_EQ( json_get_str( request, "api" ), "1" );
        EXPECT_EQ( json_get_str( request[ "params" ], "controller_uuid" ), "&0000410-uuid" );
        EXPECT_EQ( json_get_int( request[ "params" ], "instant" ), 1 );
        EXPECT_EQ( json_get_int( request[ "params" ], "queued" ), 0 );
        EXPECT_EQ( json_get_str( request[ "params" ][ "command" ], "method" ), "hub.item.value.set" );
        EXPECT_EQ( json_get_str( request[ "params" ][ "command" ][ "params" ], "_id" ), "item-1.1.2" );
        EXPECT_EQ( json_get_int( request[ "params" ][ "command" ][ "params" ], "value" ), 30 );
        auto internalId = json_get_str( request[ "params" ][ "command" ], "id" );
        auto devices = generateDevices( "", { "device-1.1", "device-1.2", "device-1.3" } );
        sendResponse( request,
                      makeRawCommandResult( internalId, "hub.item.value.set", JSON_EMPTY_OBJ ),
                      JSON_EMPTY_OBJ,
                      JSON( "type"
                            << "controller"
                            << "uuid"
                            << "&0000410-uuid" ) );
    }

    std::unique_lock< std::mutex > responseLock( responseMutex );
    EXPECT_EQ( responseCondVar.wait_for( responseLock, std::chrono::milliseconds( 500 ) ), std::cv_status::no_timeout );
    EXPECT_EQ( "{}", actualResponse.dump( 4 ) );
}

TEST_F( ExternalControllersTest, setDeviceArmed ) {  // 4.2

    std::string expectedResult = "{\n"
                                 "    \"_id\": \"device-1.2\"\n"
                                 "}";

    setCredentials( 420, "&0000420", "&0000420-uuid", { "&0000420-key-1", "&0000420-key-2" }, FwCertsType::Broken );
    std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
    nma().setCloudMethod( "cloud.controller_raw_command", [&]( rpc::Message& rpcMessage ) {
        JsonObject& request = rpcMessage.getMessage();
        EXPECT_EQ( json_get_str( request, "method" ), "cloud.controller_raw_command" );
        EXPECT_EQ( json_get_str( request, "api" ), "1" );
        EXPECT_EQ( json_get_str( request[ "params" ], "controller_uuid" ), "&0000420-uuid" );
        EXPECT_EQ( json_get_str( request[ "params" ][ "command" ], "method" ), "hub.device.armed.set" );
        EXPECT_EQ( json_get_str( request[ "params" ][ "command" ][ "params" ], "_id" ), "device-1.2" );
        EXPECT_EQ( json_get_bool( request[ "params" ][ "command" ][ "params" ], "armed" ), true );
        auto internalId = json_get_str( request[ "params" ][ "command" ], "id" );

        JsonObject& joAnswer = rpcMessage.getAnswer();
        joAnswer[ "method" ] = json_get_str( request, "method" );
        joAnswer[ "result" ] = PARSE_JSON( "{\n"
                                           "    \"action_used\": \"instant\",\n"
                                           "    \"controller_response\": {\n"
                                           "        \"api\": \"1.0\",\n"
                                           "        \"error\": null,\n"
                                           "        \"id\": \"" +
                                           internalId +
                                           "\",\n"
                                           "        \"method\": \"hub.device.armed.set\",\n"
                                           "        \"result\": " +
                                           expectedResult +
                                           "\n"
                                           "    },\n"
                                           "    \"state\": 1\n"
                                           "}" );
        joAnswer[ "error" ] = nullptr;
        joAnswer[ "sender" ] = PARSE_JSON( "{\n"
                                           "    \"type\": \"controller\",\n"
                                           "    \"uuid\": \"&0000410-uuid\"\n"
                                           "  }" );
    } );

    auto actualResult = externalControllers().setDeviceArmed( "&0000420", "device-1.2", true );
    EXPECT_EQ( expectedResult, actualResult.dump( 4 ) );
}

TEST_F( ExternalControllersTest, hubBridgeControllerConnectionsAdd ) {  // 6.1
    setCredentials( 610, "", "", {} );

    // Add IPs
    {
        auto response = setBridgeControllerConnection( "6fc13be5-19e4-4bdc-b593-7cd86869ea32",
                                                       { "192.168.0.1", "192.168.10.110" } );
        EXPECT_EQ( "{}", response[ "result" ].dump( 4 ) );
    }

    // Test
    {
        auto response = getBridgeConnectionsList();
        EXPECT_EQ( "{\n"
                   "    \"controllers\": [\n"
                   "        {\n"
                   "            \"_id\": \"6fc13be5-19e4-4bdc-b593-7cd86869ea32\",\n"
                   "            \"ip\": [\n"
                   "                \"192.168.0.1\",\n"
                   "                \"192.168.10.110\"\n"
                   "            ],\n"
                   "            \"serial\": \"\",\n"
                   "            \"status\": \"not_connected\"\n"
                   "        }\n"
                   "    ]\n"
                   "}",
                   response[ "result" ].dump( 4 ) );
    }
}

TEST_F( ExternalControllersTest, hubBridgeControllerConnectionsUpdate ) {  // 6.2
    setCredentials( 620, "", "", {} );

    // Add IPs
    {
        auto response = setBridgeControllerConnection( "6fc13be5-19e4-4bdc-b593-7cd86869ea32",
                                                       { "192.168.0.1", "192.168.10.110" } );
        EXPECT_EQ( "{}", response[ "result" ].dump( 4 ) );
    }

    // Update IPs
    {
        auto response = setBridgeControllerConnection( "6fc13be5-19e4-4bdc-b593-7cd86869ea32", { "192.168.0.2" } );
        EXPECT_EQ( "{}", response[ "result" ].dump( 4 ) );
    }

    // Test
    {
        auto response = getBridgeConnectionsList();
        EXPECT_EQ( "{\n"
                   "    \"controllers\": [\n"
                   "        {\n"
                   "            \"_id\": \"6fc13be5-19e4-4bdc-b593-7cd86869ea32\",\n"
                   "            \"ip\": [\n"
                   "                \"192.168.0.2\"\n"
                   "            ],\n"
                   "            \"serial\": \"\",\n"
                   "            \"status\": \"not_connected\"\n"
                   "        }\n"
                   "    ]\n"
                   "}",
                   response[ "result" ].dump( 4 ) );
    }
}

TEST_F( ExternalControllersTest, hubBridgeControllerConnectionsRemove ) {  // 6.3
    setCredentials( 630, "", "", {} );

    // Add IPs
    {
        auto response = setBridgeControllerConnection( "6fc13be5-19e4-4bdc-b593-7cd86869ea32",
                                                       { "192.168.0.1", "192.168.10.110" } );
        EXPECT_EQ( "{}", response[ "result" ].dump( 4 ) );
    }

    // Remove IPs
    {
        auto response = setBridgeControllerConnection( "6fc13be5-19e4-4bdc-b593-7cd86869ea32", {} );
        EXPECT_EQ( "{}", response[ "result" ].dump( 4 ) );
    }

    // Test
    {
        auto response = getBridgeConnectionsList();
        EXPECT_EQ( "{\n"
                   "    \"controllers\": []\n"
                   "}",
                   response[ "result" ].dump( 4 ) );
    }
}

TEST_F( ExternalControllersTest, hubBridgeControllerConnectionsWithCredentials ) {  // 6.4
    setCredentials( 640, "&0000640", "6fc13be5-19e4-4bdc-b593-7cd86869ea32", { "&0000640-key-1", "&0000640-key-2" } );

    // Add IPs
    {
        auto response = setBridgeControllerConnection( "6fc13be5-19e4-4bdc-b593-7cd86869ea32",
                                                       { "192.168.0.1", "192.168.10.110" } );
        EXPECT_EQ( "{}", response[ "result" ].dump( 4 ) );
    }

    // Test
    {
        auto response = getBridgeConnectionsList();
        EXPECT_EQ( "{\n"
                   "    \"controllers\": [\n"
                   "        {\n"
                   "            \"_id\": \"6fc13be5-19e4-4bdc-b593-7cd86869ea32\",\n"
                   "            \"ip\": [\n"
                   "                \"192.168.0.1\",\n"
                   "                \"192.168.10.110\"\n"
                   "            ],\n"
                   "            \"serial\": \"&0000640\",\n"
                   "            \"status\": \"not_connected\"\n"
                   "        }\n"
                   "    ]\n"
                   "}",
                   response[ "result" ].dump( 4 ) );
    }
}

TEST_F( ExternalControllersTest, hubBridgeControllerConnectionsOnlyCredentials ) {  // 6.5
    setCredentials( 650, "&0000650", "6fc13be5-19e4-4bdc-b593-7cd86869ea32", { "&0000650-key-1", "&0000650-key-2" } );

    // Clear IPs
    {
        auto response = setBridgeControllerConnection( "6fc13be5-19e4-4bdc-b593-7cd86869ea32", {} );
        EXPECT_EQ( "{}", response[ "result" ].dump( 4 ) );
    }

    // Test
    {
        auto response = getBridgeConnectionsList();
        EXPECT_EQ( "{\n"
                   "    \"controllers\": [\n"
                   "        {\n"
                   "            \"_id\": \"6fc13be5-19e4-4bdc-b593-7cd86869ea32\",\n"
                   "            \"ip\": [],\n"
                   "            \"serial\": \"&0000650\",\n"
                   "            \"status\": \"not_connected\"\n"
                   "        }\n"
                   "    ]\n"
                   "}",
                   response[ "result" ].dump( 4 ) );
    }
}

TEST_F( ExternalControllersTest, setItemValueWithBigData ) {  // 4.1

    std::mutex responseMutex;
    std::condition_variable responseCondVar;
    Json actualResponse;

    setCredentials( 410, "&0000410", "&0000410-uuid", { "&0000410-key-1", "&0000410-key-2" }, FwCertsType::Broken );
    std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
    externalControllers().setItemValue( "&0000410",
                                        JSON( "_id"
                                              << "item-1.1.2"
                                              << "value" << getBigString() ),
                                        [&]( const JsonObject& response ) {
                                            actualResponse = response;
                                            EXPECT_FALSE( response.empty() );
                                            auto error = response[ "error" ];

                                            EXPECT_EQ( json_get_int( error, "code" ), -32603 );
                                            EXPECT_EQ( json_get_str( error, "data" ), "bus.message.too.long" );
                                            EXPECT_EQ( json_get_str( error, "message" ), "Bus message is too long" );
                                            auto r = response.dump();

                                            (void) r;

                                            responseCondVar.notify_one();
                                        } );
}

}  // namespace Tests
}  // namespace HubPlatform
