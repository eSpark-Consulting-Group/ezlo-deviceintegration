#include "cloud_devices_tests/tests/fixtures/common_cloud_devices_fixture.h"
#include "cloud_devices_tests/application.h"

#include "json_utils/json_utils.h"
#include "jsonrpc/message.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace HubPlatform {
namespace Tests {

/*

TESTPLAN

DONE    1. Emit device_added event
DONE        1.1 Handling command result
DONE        1.2 Handling broadcast
DONE    2. Emit device_removed event
DONE        2.1 Handling command result
DONE        2.2 Handling broadcast
DONE    3. Emit device_updated event
DONE        3.1 Handling command result
DONE        3.2 Handling broadcast
DONE    4. Handle get_all_devices_ids command
DONE    5. Handle get_device command
DONE    6. Handle send_command command
DONE    7. Emit command_response event
DONE        7.1 Timeout
DONE        7.2 Cloud error
DONE        7.3 Unknown device error
DONE        7.4 Unknown capability error
DONE        7.5 Correct
DONE    8. Cloud connectivity
DONE        8.1 Mark all devices as unreachable on unregister
DONE        8.2 Restore all devices reachable state on register

 */

class CloudDevicesBindingsTest : public CommonCloudDevicesTests {
public:
    void SetUp() override {
        CommonCloudDevicesTests::SetUp();

        // This all should be already handled in the test itself
        setDevicesCommand( JSON_EMPTY_OBJ, __FILE__, __LINE__ );
        ignoreBroadcasts(
            []( const JsonObject& broadcast ) { return isTestBroadcast( broadcast, __FILE__, __LINE__ ); },
            __FILE__,
            __LINE__,
            std::chrono::milliseconds( 10 ) );
    }
    void ignoreBroadcastsLoud( const char* file, int line ) {
        ignoreBroadcasts(
            [file, line]( const JsonObject& broadcast ) {
                LOGGER_ERROR_RAW( "#     Unhandled broadcast on start of a test: " + cc::j( broadcast ) );
                ADD_FAILURE_AT( file, line ) << "unhandled broadcast: " << cc::j( broadcast );
                return isTestBroadcast( broadcast, file, line );
            },
            file,
            line,
            std::chrono::milliseconds( 100 ) );
    }

    void setDevicesCommand( const Json& devices, const char* file, int line ) {
        sendCloudRegistered();

        auto request = waitForRequest( file, line );
        if( json_get_str( request, "method" ) != "cloud.controller_abstracts_list" ) {
            ADD_FAILURE_AT( file, line ) << "Got request with method:" << json_get_str( request, "method" )
                                         << ", expected cloud.controller_abstracts_list";
            throw std::exception();
        }
        setCurrentVersion( json_get_int( request[ "params" ], "version" ) );
        increaseCurrentVersion();
        Json result;
        result[ "version" ] = getCurrentVersion();
        result[ "devices" ] = devices;
        sendResponse( request, result );
    }

    void sendAddDevicesBroadcast( const Json& devices ) {
        increaseCurrentVersion();
        Json result;
        result[ "version" ] = getCurrentVersion();
        result[ "devices" ] = devices;
        sendBroadcast( "cloud.abstracts.added", result );
    }

    void sendUpdateDevicesBroadcast( const Json& devices ) {
        increaseCurrentVersion();
        Json result;
        result[ "version" ] = getCurrentVersion();
        result[ "devices" ] = devices;
        sendBroadcast( "cloud.abstracts.updated", result );
    }

    void sendRemoveDevicesBroadcast( const JsonArray& devicesIds ) {
        increaseCurrentVersion();
        Json result;
        result[ "version" ] = getCurrentVersion();
        result[ "devices" ] = devicesIds;
        sendBroadcast( "cloud.abstracts.deleted", result );
    }

    void triggerTestPluginCommandByAddingDeviceWithId( const std::string& command ) {
        increaseCurrentVersion();
        Json dummyDevices;
        dummyDevices[ command ] = JSON( "id" << command );
        Json result;
        result[ "version" ] = getCurrentVersion();
        result[ "devices" ] = dummyDevices;
        sendBroadcast( "cloud.abstracts.added", result );
    }

    Json waitForTestBroadcast( const char* file, int line ) {
        Json broadcast = waitForBroadcast( "hub.extensions.plugin.ui_broadcast", file, line );
        if( !isTestBroadcast( broadcast, file, line ) ) {
            throw std::exception();
        }
        return broadcast[ "result" ][ "data" ];
    }

private:
    static bool isTestBroadcast( const Json& broadcast, const char* file, int line ) {
        if( json_get_str( broadcast, "msg_subclass" ) != "hub.extensions.plugin.ui_broadcast" ) {
            ADD_FAILURE_AT( file, line ) << "Got broadcast with msg_class:" << json_get_str( broadcast, "msg_subclass" )
                                         << ", expected hub.extensions.plugin.ui_broadcast";
            return false;
        }
        if( json_get_str( broadcast[ "result" ], "plugin" ) != "cloud_devices" ) {
            ADD_FAILURE_AT( file, line ) << "Got broadcast from plugin:"
                                         << json_get_str( broadcast[ "result" ], "plugin" )
                                         << ", expected from cloud_devices";
            return false;
        }
        return true;
    }

private:
    int m_currentVersion;
};

#define TEST_SEND_DEVICES_LIST_COMMAND( devices ) setDevicesCommand( devices, __FILE__, __LINE__ )
#define TEST_WAIT_FOR_TEST_BROADCAST() waitForTestBroadcast( __FILE__, __LINE__ )

TEST_F( CloudDevicesBindingsTest, EmitDeviceAddedEventHandlingCommandResult ) {  // 1.1

    TEST_SEND_DEVICES_LIST_COMMAND(
        PARSE_JSON( "{\n"
                    "    \"test.event_params<1.1-1>\": { \"id\": \"test.event_params<1.1-1>\" },\n"
                    "    \"test.event_params<1.1-2>\": { \"id\": \"test.event_params<1.1-2>\" }\n"
                    "}" ) );

    {
        auto broadcast = TEST_WAIT_FOR_TEST_BROADCAST();
        EXPECT_EQ( "{\n"
                   "    \"event\": \"device_added\",\n"
                   "    \"id\": \"test.event_params<1.1-1>\"\n"
                   "}",
                   broadcast.dump( 4 ) );
    }

    {
        auto broadcast = TEST_WAIT_FOR_TEST_BROADCAST();
        EXPECT_EQ( "{\n"
                   "    \"event\": \"device_added\",\n"
                   "    \"id\": \"test.event_params<1.1-2>\"\n"
                   "}",
                   broadcast.dump( 4 ) );
    }

    // clean-up
    setDevicesCommand( JSON_EMPTY_OBJ, __FILE__, __LINE__ );
    Json resultData;
    resultData = waitForTestBroadcast( __FILE__, __LINE__ );
    EXPECT_EQ( R"({"event": "device_removed","id": "test.event_params<1.1-1>"})"_json, resultData );
    resultData = waitForTestBroadcast( __FILE__, __LINE__ );
    EXPECT_EQ( R"({"event": "device_removed","id": "test.event_params<1.1-2>"})"_json, resultData );
}

TEST_F( CloudDevicesBindingsTest, EmitDeviceAddedEventHandlingBroadcast ) {  // 1.2

    sendAddDevicesBroadcast( PARSE_JSON( "{\n"
                                         "    \"test.event_params<1.2-1>\": { \"id\": \"test.event_params<1.2-1>\" },\n"
                                         "    \"test.event_params<1.2-2>\": { \"id\": \"test.event_params<1.2-2>\" }\n"
                                         "}" ) );

    {
        auto broadcast = TEST_WAIT_FOR_TEST_BROADCAST();
        EXPECT_EQ( "{\n"
                   "    \"event\": \"device_added\",\n"
                   "    \"id\": \"test.event_params<1.2-1>\"\n"
                   "}",
                   broadcast.dump( 4 ) );
    }

    {
        auto broadcast = TEST_WAIT_FOR_TEST_BROADCAST();
        EXPECT_EQ( "{\n"
                   "    \"event\": \"device_added\",\n"
                   "    \"id\": \"test.event_params<1.2-2>\"\n"
                   "}",
                   broadcast.dump( 4 ) );
    }

    // clean-up
    setDevicesCommand( JSON_EMPTY_OBJ, __FILE__, __LINE__ );
    Json resultData;
    resultData = waitForTestBroadcast( __FILE__, __LINE__ );
    EXPECT_EQ( R"({"event": "device_removed","id": "test.event_params<1.2-1>"})"_json, resultData );
    resultData = waitForTestBroadcast( __FILE__, __LINE__ );
    EXPECT_EQ( R"({"event": "device_removed","id": "test.event_params<1.2-2>"})"_json, resultData );
}

TEST_F( CloudDevicesBindingsTest, EmitDeviceRemovedEventHandlingCommandResult ) {  // 2.1

    // Prepare

    TEST_SEND_DEVICES_LIST_COMMAND(
        PARSE_JSON( "{\n"
                    "    \"test.event_params<2.1-1>\": { \"id\": \"test.event_params<2.1-1>\" },\n"
                    "    \"test.event_params<2.1-2>\": { \"id\": \"test.event_params<2.1-2>\" }\n"
                    "}" ) );

    TEST_WAIT_FOR_TEST_BROADCAST();
    TEST_WAIT_FOR_TEST_BROADCAST();

    // Test

    TEST_SEND_DEVICES_LIST_COMMAND( JSON_EMPTY_OBJ );

    {
        auto broadcast = TEST_WAIT_FOR_TEST_BROADCAST();
        EXPECT_EQ( "{\n"
                   "    \"event\": \"device_removed\",\n"
                   "    \"id\": \"test.event_params<2.1-1>\"\n"
                   "}",
                   broadcast.dump( 4 ) );
    }

    {
        auto broadcast = TEST_WAIT_FOR_TEST_BROADCAST();
        EXPECT_EQ( "{\n"
                   "    \"event\": \"device_removed\",\n"
                   "    \"id\": \"test.event_params<2.1-2>\"\n"
                   "}",
                   broadcast.dump( 4 ) );
    }
    ignoreBroadcastsLoud( __FILE__, __LINE__ );
}

TEST_F( CloudDevicesBindingsTest, EmitDeviceRemovedEventHandlingBroadcast ) {  // 2.2

    // Prepare

    TEST_SEND_DEVICES_LIST_COMMAND(
        PARSE_JSON( "{\n"
                    "    \"test.event_params<2.2-1>\": { \"id\": \"test.event_params<2.2-1>\" },\n"
                    "    \"test.event_params<2.2-2>\": { \"id\": \"test.event_params<2.2-2>\" }\n"
                    "}" ) );

    TEST_WAIT_FOR_TEST_BROADCAST();
    TEST_WAIT_FOR_TEST_BROADCAST();

    // Test

    sendRemoveDevicesBroadcast( PARSE_JSON( "[ \"test.event_params<2.2-1>\", \"test.event_params<2.2-2>\" ]" ) );

    {
        auto broadcast = TEST_WAIT_FOR_TEST_BROADCAST();
        EXPECT_EQ( "{\n"
                   "    \"event\": \"device_removed\",\n"
                   "    \"id\": \"test.event_params<2.2-1>\"\n"
                   "}",
                   broadcast.dump( 4 ) );
    }

    {
        auto broadcast = TEST_WAIT_FOR_TEST_BROADCAST();
        EXPECT_EQ( "{\n"
                   "    \"event\": \"device_removed\",\n"
                   "    \"id\": \"test.event_params<2.2-2>\"\n"
                   "}",
                   broadcast.dump( 4 ) );
    }
    ignoreBroadcastsLoud( __FILE__, __LINE__ );
}

TEST_F( CloudDevicesBindingsTest, EmitDeviceUpdatedEventHandlingCommandResult ) {  // 3.1

    // Prepare

    TEST_SEND_DEVICES_LIST_COMMAND( PARSE_JSON( "{\n"
                                                "    \"test.event_params<3.1-1>\": {\n"
                                                "        \"id\": \"test.event_params<3.1-1>\",\n"
                                                "        \"boolKey\": true,\n"
                                                "        \"intKey\": 0,\n"
                                                "        \"floatKey\": 0.25,\n"
                                                "        \"stringKey\": \"stringVal\",\n"
                                                "        \"arrayKey\": [ \"item1\", \"item2\" ]\n"
                                                "    },\n"
                                                "    \"test.event_params<3.1-2>\": {\n"
                                                "        \"id\": \"test.event_params<3.1-2>\",\n"
                                                "        \"objectKey\": {\n"
                                                "            \"subKey1\": \"subValue1\",\n"
                                                "            \"subKey2\": \"subValue2\"\n"
                                                "        },\n"
                                                "        \"persistentKey\": \"persistentVal\"\n"
                                                "    }\n"
                                                "}" ) );

    TEST_WAIT_FOR_TEST_BROADCAST();
    TEST_WAIT_FOR_TEST_BROADCAST();

    // Test

    TEST_SEND_DEVICES_LIST_COMMAND( PARSE_JSON( "{\n"
                                                "    \"test.event_params<3.1-1>\": {\n"
                                                "        \"id\": \"test.event_params<3.1-1>\",\n"
                                                "        \"boolKey\": false,\n"
                                                "        \"intKey\": 42,\n"
                                                "        \"floatKey\": 2.75,\n"
                                                "        \"stringKey\": \"newStringVal\",\n"
                                                "        \"arrayKey\": [ \"item2\", \"item3\" ]\n"
                                                "    },\n"
                                                "    \"test.event_params<3.1-2>\": {\n"
                                                "        \"id\": \"test.event_params<3.1-2>\",\n"
                                                "        \"objectKey\": {\n"
                                                "            \"subKey2\": \"subValue2\",\n"
                                                "            \"subKey3\": \"subValue3\"\n"
                                                "        },\n"
                                                "        \"persistentKey\": \"persistentVal\"\n"
                                                "    }\n"
                                                "}" ) );

    {
        auto broadcast = TEST_WAIT_FOR_TEST_BROADCAST();
        EXPECT_EQ( "{\n"
                   "    \"arrayKey\": [\n"
                   "        \"item2\",\n"
                   "        \"item3\"\n"
                   "    ],\n"
                   "    \"boolKey\": false,\n"
                   "    \"event\": \"device_updated\",\n"
                   "    \"floatKey\": 2.75,\n"
                   "    \"id\": \"test.event_params<3.1-1>\",\n"
                   "    \"intKey\": 42,\n"
                   "    \"stringKey\": \"newStringVal\"\n"
                   "}",
                   broadcast.dump( 4 ) );
    }

    {
        auto broadcast = TEST_WAIT_FOR_TEST_BROADCAST();
        EXPECT_EQ( "{\n"
                   "    \"event\": \"device_updated\",\n"
                   "    \"id\": \"test.event_params<3.1-2>\",\n"
                   "    \"objectKey\": {\n"
                   "        \"subKey2\": \"subValue2\",\n"
                   "        \"subKey3\": \"subValue3\"\n"
                   "    }\n"
                   "}",
                   broadcast.dump( 4 ) );
    }

    // clean-up
    setDevicesCommand( JSON_EMPTY_OBJ, __FILE__, __LINE__ );
    Json resultData;
    resultData = waitForTestBroadcast( __FILE__, __LINE__ );
    EXPECT_EQ( R"({"event": "device_removed","id": "test.event_params<3.1-1>"})"_json, resultData );
    resultData = waitForTestBroadcast( __FILE__, __LINE__ );
    EXPECT_EQ( R"({"event": "device_removed","id": "test.event_params<3.1-2>"})"_json, resultData );

    ignoreBroadcastsLoud( __FILE__, __LINE__ );
}

TEST_F( CloudDevicesBindingsTest, EmitDeviceUpdatedEventHandlingBroadcast ) {  // 3.2

    // Prepare

    TEST_SEND_DEVICES_LIST_COMMAND( PARSE_JSON( "{\n"
                                                "    \"test.event_params<3.2-1>\": {\n"
                                                "        \"id\": \"test.event_params<3.2-1>\",\n"
                                                "        \"boolKey\": true,\n"
                                                "        \"intKey\": 0,\n"
                                                "        \"floatKey\": 0.25,\n"
                                                "        \"stringKey\": \"stringVal\",\n"
                                                "        \"arrayKey\": [ \"item1\", \"item2\" ]\n"
                                                "    },\n"
                                                "    \"test.event_params<3.2-2>\": {\n"
                                                "        \"id\": \"test.event_params<3.2-2>\",\n"
                                                "        \"objectKey\": {\n"
                                                "            \"subKey1\": \"subValue1\",\n"
                                                "            \"subKey2\": \"subValue2\"\n"
                                                "        },\n"
                                                "        \"persistentKey\": \"persistentVal\"\n"
                                                "    }\n"
                                                "}" ) );

    TEST_WAIT_FOR_TEST_BROADCAST();
    TEST_WAIT_FOR_TEST_BROADCAST();

    // Test

    sendUpdateDevicesBroadcast( PARSE_JSON( "{\n"
                                            "    \"test.event_params<3.2-1>\": {\n"
                                            "        \"id\": \"test.event_params<3.2-1>\",\n"
                                            "        \"boolKey\": false,\n"
                                            "        \"intKey\": 42,\n"
                                            "        \"floatKey\": 2.75,\n"
                                            "        \"stringKey\": \"newStringVal\",\n"
                                            "        \"arrayKey\": [ \"item2\", \"item3\" ]\n"
                                            "    },\n"
                                            "    \"test.event_params<3.2-2>\": {\n"
                                            "        \"id\": \"test.event_params<3.2-2>\",\n"
                                            "        \"objectKey\": {\n"
                                            "            \"subKey2\": \"subValue2\",\n"
                                            "            \"subKey3\": \"subValue3\"\n"
                                            "        }\n"
                                            "    }\n"
                                            "}" ) );

    {
        auto broadcast = TEST_WAIT_FOR_TEST_BROADCAST();
        EXPECT_EQ( "{\n"
                   "    \"arrayKey\": [\n"
                   "        \"item2\",\n"
                   "        \"item3\"\n"
                   "    ],\n"
                   "    \"boolKey\": false,\n"
                   "    \"event\": \"device_updated\",\n"
                   "    \"floatKey\": 2.75,\n"
                   "    \"id\": \"test.event_params<3.2-1>\",\n"
                   "    \"intKey\": 42,\n"
                   "    \"stringKey\": \"newStringVal\"\n"
                   "}",
                   broadcast.dump( 4 ) );
    }

    {
        auto broadcast = TEST_WAIT_FOR_TEST_BROADCAST();
        EXPECT_EQ( "{\n"
                   "    \"event\": \"device_updated\",\n"
                   "    \"id\": \"test.event_params<3.2-2>\",\n"
                   "    \"objectKey\": {\n"
                   "        \"subKey2\": \"subValue2\",\n"
                   "        \"subKey3\": \"subValue3\"\n"
                   "    }\n"
                   "}",
                   broadcast.dump( 4 ) );
    }

    // clean-up
    setDevicesCommand( JSON_EMPTY_OBJ, __FILE__, __LINE__ );
    Json resultData;
    resultData = waitForTestBroadcast( __FILE__, __LINE__ );
    EXPECT_EQ( R"({"event": "device_removed","id": "test.event_params<3.2-1>"})"_json, resultData );
    resultData = waitForTestBroadcast( __FILE__, __LINE__ );
    EXPECT_EQ( R"({"event": "device_removed","id": "test.event_params<3.2-2>"})"_json, resultData );

    ignoreBroadcastsLoud( __FILE__, __LINE__ );
}

TEST_F( CloudDevicesBindingsTest, HandleGetAllDevicesIdsCommand ) {  // 4

    // Prepare

    TEST_SEND_DEVICES_LIST_COMMAND( PARSE_JSON( "{\n"
                                                "    \"4-1\": { \"id\": \"4-1\" },\n"
                                                "    \"4-2\": { \"id\": \"4-2\" }\n"
                                                "}" ) );

    // Test

    triggerTestPluginCommandByAddingDeviceWithId( "test.get_all_devices_ids<4-3>()" );

    {
        auto broadcast = TEST_WAIT_FOR_TEST_BROADCAST();
        EXPECT_EQ( "[\n"
                   "    \"4-1\",\n"
                   "    \"4-2\",\n"
                   "    \"test.get_all_devices_ids<4-3>()\"\n"
                   "]",
                   broadcast.dump( 4 ) );
    }

    // clean-up
    setDevicesCommand( JSON_EMPTY_OBJ, __FILE__, __LINE__ );
    auto resultData = TEST_WAIT_FOR_TEST_BROADCAST();
    EXPECT_EQ( resultData, JSON_EMPTY_OBJ );

    ignoreBroadcastsLoud( __FILE__, __LINE__ );
}

TEST_F( CloudDevicesBindingsTest, HandleGetDeviceCommand ) {  // 5

    // Prepare

    TEST_SEND_DEVICES_LIST_COMMAND( PARSE_JSON( "{\n"
                                                "    \"5-1\": {\n"
                                                "        \"id\": \"5-1\",\n"
                                                "        \"key1\": \"value1\",\n"
                                                "        \"key2\": \"value2\"\n"
                                                "    },\n"
                                                "    \"5-2\": {\n"
                                                "        \"id\": \"5-2\",\n"
                                                "        \"key2\": \"value2\",\n"
                                                "        \"key3\": \"value3\"\n"
                                                "    }\n"
                                                "}" ) );

    // Test

    triggerTestPluginCommandByAddingDeviceWithId( "test.get_device<5-3>(5-2)" );

    {
        auto broadcast = TEST_WAIT_FOR_TEST_BROADCAST();
        EXPECT_EQ( "{\n"
                   "    \"id\": \"5-2\",\n"
                   "    \"key2\": \"value2\",\n"
                   "    \"key3\": \"value3\"\n"
                   "}",
                   broadcast.dump( 4 ) );
    }

    // clean-up
    setDevicesCommand( JSON_EMPTY_OBJ, __FILE__, __LINE__ );

    ignoreBroadcastsLoud( __FILE__, __LINE__ );
}

TEST_F( CloudDevicesBindingsTest, HandleSendCommandCommand ) {  // 6

    // Prepare

    TEST_SEND_DEVICES_LIST_COMMAND( PARSE_JSON( "{\n"
                                                "    \"6-1\": {\n"
                                                "        \"id\": \"6-1\",\n"
                                                "        \"capabilities\": [\n"
                                                "            \"text_command\",\n"
                                                "            \"ui_flag_alexa_account\"\n"
                                                "        ]\n"
                                                "    }\n"
                                                "}" ) );

    // Test

    triggerTestPluginCommandByAddingDeviceWithId(
        R"(test.send_command<6-2>('cmd_id', '6-1', 'text_command', 'send', '{text="turn on the light"}'))" );

    {
        auto request = TEST_WAIT_FOR_REQUEST();

        EXPECT_EQ( request[ "method" ], "cloud.controller_abstracts_set" );
        EXPECT_EQ( "{\n"
                   "    \"capability\": \"text_command\",\n"
                   "    \"command\": \"send\",\n"
                   "    \"parameters\": {\n"
                   "        \"text\": \"turn on the light\"\n"
                   "    },\n"
                   "    \"uuid\": \"6-1\"\n"
                   "}",
                   request[ "params" ].dump( 4 ) );
        sendResponse( request );
    }
    // clean-up
    setDevicesCommand( JSON_EMPTY_OBJ, __FILE__, __LINE__ );
    ignoreBroadcastsLoud( __FILE__, __LINE__ );
}

TEST_F( CloudDevicesBindingsTest, HandleCommandResponseEventTimeout ) {  // 7.1

    // Prepare

    TEST_SEND_DEVICES_LIST_COMMAND( PARSE_JSON( "{\n"
                                                "    \"7.1-1\": {\n"
                                                "        \"id\": \"7.1-1\",\n"
                                                "        \"capabilities\": [\n"
                                                "            \"text_command\",\n"
                                                "            \"ui_flag_alexa_account\"\n"
                                                "        ]\n"
                                                "    }\n"
                                                "}" ) );

    // Test

    triggerTestPluginCommandByAddingDeviceWithId(
        R"(test.send_command<7.1-2>('test.event_params<cmd_id.7.1>', '7.1-1', 'text_command', 'send', '{text="turn on the light"}'))" );

    {
        auto request = TEST_WAIT_FOR_REQUEST();
        EXPECT_EQ( request[ "method" ], "cloud.controller_abstracts_set" );
        timeoutResponse();
        sendResponse( request );
    }

    {
        auto broadcast = TEST_WAIT_FOR_TEST_BROADCAST();
        EXPECT_EQ( "{\n"
                   "    \"command_id\": \"test.event_params<cmd_id.7.1>\",\n"
                   "    \"error\": {\n"
                   "        \"code\": -1,\n"
                   "        \"data\": \"cloud_devices.error.request_timeout\",\n"
                   "        \"description\": \"\"\n"
                   "    },\n"
                   "    \"event\": \"command_response\",\n"
                   "    \"result\": {}\n"
                   "}",
                   broadcast.dump( 4 ) );
    }
    // clean-up
    setDevicesCommand( JSON_EMPTY_OBJ, __FILE__, __LINE__ );
    ignoreBroadcastsLoud( __FILE__, __LINE__ );
}

TEST_F( CloudDevicesBindingsTest, HandleCommandResponseEventCloudError ) {  // 7.2

    // Prepare

    TEST_SEND_DEVICES_LIST_COMMAND( PARSE_JSON( "{\n"
                                                "    \"7.2-1\": {\n"
                                                "        \"id\": \"7.2-1\",\n"
                                                "        \"capabilities\": [\n"
                                                "            \"text_command\",\n"
                                                "            \"ui_flag_alexa_account\"\n"
                                                "        ]\n"
                                                "    }\n"
                                                "}" ) );

    // Test

    triggerTestPluginCommandByAddingDeviceWithId(
        R"(test.send_command<7.2-2>('test.event_params<cmd_id.7.2>', '7.2-1', 'text_command', 'send', '{text="turn on the light"}'))" );

    {
        auto request = TEST_WAIT_FOR_REQUEST();
        EXPECT_EQ( request[ "method" ], "cloud.controller_abstracts_set" );
        sendResponse( request,
                      JSON_EMPTY_OBJ,
                      PARSE_JSON( "{\n"
                                  "    \"code\": 11,\n"
                                  "    \"data\": \"cloud.error.method_not_found\",\n"
                                  "    \"description\": \"\"\n"
                                  "}" ) );
    }

    {
        auto broadcast = TEST_WAIT_FOR_TEST_BROADCAST();
        EXPECT_EQ( "{\n"
                   "    \"command_id\": \"test.event_params<cmd_id.7.2>\",\n"
                   "    \"error\": {\n"
                   "        \"code\": 11,\n"
                   "        \"data\": \"cloud.error.method_not_found\",\n"
                   "        \"description\": \"\"\n"
                   "    },\n"
                   "    \"event\": \"command_response\",\n"
                   "    \"result\": {}\n"
                   "}",
                   broadcast.dump( 4 ) );
    }
    // clean-up
    setDevicesCommand( JSON_EMPTY_OBJ, __FILE__, __LINE__ );
    ignoreBroadcastsLoud( __FILE__, __LINE__ );
}

TEST_F( CloudDevicesBindingsTest, HandleCommandResponseEventUnknownDeviceError ) {  // 7.3

    // Prepare

    TEST_SEND_DEVICES_LIST_COMMAND( PARSE_JSON( "{\n"
                                                "    \"7.3-1\": {\n"
                                                "        \"id\": \"7.3-1\",\n"
                                                "        \"capabilities\": [\n"
                                                "            \"text_command\",\n"
                                                "            \"ui_flag_alexa_account\"\n"
                                                "        ]\n"
                                                "    }\n"
                                                "}" ) );

    // Test

    triggerTestPluginCommandByAddingDeviceWithId(
        R"(test.send_command<7.3-2>('test.event_params<cmd_id.7.3>', '7.3-892', 'text_command', 'send', '{text="turn on the light"}'))" );

    TEST_WAIT_NO_REQUESTS();

    {
        auto broadcast = TEST_WAIT_FOR_TEST_BROADCAST();
        EXPECT_EQ( "{\n"
                   "    \"error\": \"fail to send command. command id: test.event_params<cmd_id.7.3>, device id: "
                   "7.3-892, capability: text_command, command: send. Trying to send command to unknown device\"\n"
                   "}",
                   broadcast.dump( 4 ) );
    }
    // clean-up
    setDevicesCommand( JSON_EMPTY_OBJ, __FILE__, __LINE__ );
    ignoreBroadcastsLoud( __FILE__, __LINE__ );
}

TEST_F( CloudDevicesBindingsTest, HandleCommandResponseEventUnknownCapabilityError ) {  // 7.4

    // Prepare

    TEST_SEND_DEVICES_LIST_COMMAND( PARSE_JSON( "{\n"
                                                "    \"7.4-1\": {\n"
                                                "        \"id\": \"7.4-1\",\n"
                                                "        \"capabilities\": [\n"
                                                "            \"text_command\",\n"
                                                "            \"ui_flag_alexa_account\"\n"
                                                "        ]\n"
                                                "    }\n"
                                                "}" ) );

    // Test

    triggerTestPluginCommandByAddingDeviceWithId(
        R"(test.send_command<7.4-2>('test.event_params<cmd_id.7.4>', '7.4-1', 'voice_command', 'send', '{voice="turn on the light"}'))" );

    TEST_WAIT_NO_REQUESTS();

    {
        auto broadcast = TEST_WAIT_FOR_TEST_BROADCAST();
        EXPECT_EQ( "{\n"
                   "    \"error\": \"fail to send command. command id: test.event_params<cmd_id.7.4>, device id: "
                   "7.4-1, capability: voice_command, command: send. Trying to send command to unknown capability\"\n"
                   "}",
                   broadcast.dump( 4 ) );
    }
    // clean-up
    setDevicesCommand( JSON_EMPTY_OBJ, __FILE__, __LINE__ );
    ignoreBroadcastsLoud( __FILE__, __LINE__ );
}

TEST_F( CloudDevicesBindingsTest, HandleCommandResponseEventCorrect ) {  // 7.5

    // Prepare

    TEST_SEND_DEVICES_LIST_COMMAND( PARSE_JSON( "{\n"
                                                "    \"7.5-1\": {\n"
                                                "        \"id\": \"7.5-1\",\n"
                                                "        \"capabilities\": [\n"
                                                "            \"text_command\",\n"
                                                "            \"ui_flag_alexa_account\"\n"
                                                "        ]\n"
                                                "    }\n"
                                                "}" ) );

    // Test

    triggerTestPluginCommandByAddingDeviceWithId(
        R"(test.send_command<7.5-2>('test.event_params<cmd_id.7.5>', '7.5-1', 'text_command', 'send', '{text="turn on the light"}'))" );

    {
        auto request = TEST_WAIT_FOR_REQUEST();
        EXPECT_EQ( request[ "method" ], "cloud.controller_abstracts_set" );
        sendResponse( request,
                      PARSE_JSON( "{\n"
                                  "    \"testKey\": \"testValue\"\n"
                                  "}" ) );
    }

    {
        auto broadcast = TEST_WAIT_FOR_TEST_BROADCAST();
        EXPECT_EQ( "{\n"
                   "    \"command_id\": \"test.event_params<cmd_id.7.5>\",\n"
                   "    \"error\": {},\n"
                   "    \"event\": \"command_response\",\n"
                   "    \"result\": {\n"
                   "        \"testKey\": \"testValue\"\n"
                   "    }\n"
                   "}",
                   broadcast.dump( 4 ) );
    }
    // clean-up
    setDevicesCommand( JSON_EMPTY_OBJ, __FILE__, __LINE__ );
    ignoreBroadcastsLoud( __FILE__, __LINE__ );
}

TEST_F( CloudDevicesBindingsTest, CloudConnectivityMarkAllDevicesAsUnreachableOnUnregister ) {  // 8.1

    // Prepare

    TEST_SEND_DEVICES_LIST_COMMAND( PARSE_JSON( "{\n"
                                                "    \"test.event_params<8.1-1>\": {\n"
                                                "        \"id\": \"test.event_params<8.1-1>\",\n"
                                                "        \"reachable\": 0\n"
                                                "    },\n"
                                                "    \"test.event_params<8.1-2>\": {\n"
                                                "        \"id\": \"test.event_params<8.1-2>\",\n"
                                                "        \"reachable\": 1\n"
                                                "    }\n"
                                                "}" ) );

    TEST_WAIT_FOR_TEST_BROADCAST();
    TEST_WAIT_FOR_TEST_BROADCAST();

    // Test

    sendCloudUnregistered();

    {
        auto broadcast = TEST_WAIT_FOR_TEST_BROADCAST();
        EXPECT_EQ( "{\n"
                   "    \"event\": \"device_updated\",\n"
                   "    \"id\": \"test.event_params<8.1-2>\",\n"
                   "    \"reachable\": 0\n"
                   "}",
                   broadcast.dump( 4 ) );
    }

    // Test

    triggerTestPluginCommandByAddingDeviceWithId( "test.get_device<8.1-3>(test.event_params<8.1-2>)" );

    {
        auto broadcast = TEST_WAIT_FOR_TEST_BROADCAST();
        EXPECT_EQ( "{\n"
                   "    \"id\": \"test.event_params<8.1-2>\",\n"
                   "    \"reachable\": 0\n"
                   "}",
                   broadcast.dump( 4 ) );
    }
    // clean-up
    setDevicesCommand( JSON_EMPTY_OBJ, __FILE__, __LINE__ );
    Json resultData;
    resultData = waitForTestBroadcast( __FILE__, __LINE__ );
    Json expected = R"({"event": "device_updated","id": "test.event_params<8.1-2>", "reachable": 1})"_json;
    EXPECT_EQ( expected, resultData );
    resultData = waitForTestBroadcast( __FILE__, __LINE__ );
    expected = R"({"event": "device_removed","id": "test.event_params<8.1-1>"})"_json;
    EXPECT_EQ( expected, resultData );
    resultData = waitForTestBroadcast( __FILE__, __LINE__ );
    expected = R"({"event": "device_removed","id": "test.event_params<8.1-2>"})"_json;
    EXPECT_EQ( expected, resultData );

    ignoreBroadcastsLoud( __FILE__, __LINE__ );
}

TEST_F( CloudDevicesBindingsTest, CloudConnectivityRestoreAllDevicesReachableStateOnRegister ) {  // 8.2

    // Prepare

    TEST_SEND_DEVICES_LIST_COMMAND( PARSE_JSON( "{\n"
                                                "    \"test.event_params<8.2-1>\": {\n"
                                                "        \"id\": \"test.event_params<8.2-1>\",\n"
                                                "        \"reachable\": 0\n"
                                                "    },\n"
                                                "    \"test.event_params<8.2-2>\": {\n"
                                                "        \"id\": \"test.event_params<8.2-2>\",\n"
                                                "        \"reachable\": 1\n"
                                                "    }\n"
                                                "}" ) );

    TEST_WAIT_FOR_TEST_BROADCAST();
    TEST_WAIT_FOR_TEST_BROADCAST();

    sendCloudUnregistered();

    TEST_WAIT_FOR_TEST_BROADCAST();

    // Test

    sendCloudRegistered();
    // Prepare

    {
        auto request = TEST_WAIT_FOR_REQUEST();

        EXPECT_EQ( request[ "method" ], "cloud.controller_abstracts_list" );
        EXPECT_EQ( request[ "api" ], "1" );
        setCurrentVersion( json_get_int( request[ "params" ], "version" ) );
        sendResponse(
            request,
            PARSE_JSON( R"({ "version": )" + std::to_string( getCurrentVersion() ) + R"(, "not_modified": 1 })" ) );
    }

    {
        auto broadcast = TEST_WAIT_FOR_TEST_BROADCAST();
        EXPECT_EQ( "{\n"
                   "    \"event\": \"device_updated\",\n"
                   "    \"id\": \"test.event_params<8.2-2>\",\n"
                   "    \"reachable\": 1\n"
                   "}",
                   broadcast.dump( 4 ) );
    }

    // Test

    triggerTestPluginCommandByAddingDeviceWithId( "test.get_device<8.2-3>(test.event_params<8.2-2>)" );

    {
        auto broadcast = TEST_WAIT_FOR_TEST_BROADCAST();
        EXPECT_EQ( "{\n"
                   "    \"id\": \"test.event_params<8.2-2>\",\n"
                   "    \"reachable\": 1\n"
                   "}",
                   broadcast.dump( 4 ) );
    }

    // clean-up
    setDevicesCommand( JSON_EMPTY_OBJ, __FILE__, __LINE__ );
    Json resultData;
    resultData = waitForTestBroadcast( __FILE__, __LINE__ );
    EXPECT_EQ( R"({"event": "device_updated","id": "test.event_params<8.2-2>", "reachable": 1})"_json, resultData );
    resultData = waitForTestBroadcast( __FILE__, __LINE__ );
    EXPECT_EQ( R"({"event": "device_removed","id": "test.event_params<8.2-1>"})"_json, resultData );
    resultData = waitForTestBroadcast( __FILE__, __LINE__ );
    EXPECT_EQ( R"({"event": "device_removed","id": "test.event_params<8.2-2>"})"_json, resultData );

    ignoreBroadcastsLoud( __FILE__, __LINE__ );
}

}  // namespace Tests
}  // namespace HubPlatform
