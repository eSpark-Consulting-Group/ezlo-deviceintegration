#include "cloud_devices_tests/tests/fixtures/common_cloud_devices_fixture.h"
#include "cloud_devices_tests/application.h"

#include "config_reader/config_reader.h"
#include "json_utils/json_utils.h"
#include "jsonrpc/message.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace HubPlatform {
namespace Tests {

/*

TESTPLAN

DONE    1. Request cloud devices
DONE        1.1 After cloud registration
TODO        1.2 After restart if already registered on cloud
DONE    2. Handle cloud devices
DONE        2.1 Re-request on error
DONE        2.2 Re-request on timeout
DONE        2.3 Correct response
DONE    3. Handle cloud device events
DONE        3.1 Added
DONE            3.1.1 Correct
DONE            3.1.2 Existing device
DONE        3.2 Deleted
DONE            3.2.1 Correct
DONE            3.2.2 Unknown device
DONE        3.3 Updated
DONE            3.3.1 Correct
DONE            3.3.2 Unknown device
DONE        3.4 Without version
DONE        3.5 With version that differs from current not by +1

 */

class CloudDevicesAddonTest : public CommonCloudDevicesTests {
public:
    void SetUp() override {
        CommonCloudDevicesTests::SetUp();
    }

    static void reconnectToNma() {
        nma().sendBroadcast( HUB_NETWORK_DISCONNECTED, {} );
        nma().sendBroadcast( HUB_NETWORK_CONNECTED, {} );
    }

    std::vector< std::string > waitForNewDevicesInDb( int version, const char* file = "", int line = 0 ) {
        waitForDbEntry( "db.addons.cloud_devices.versions", JSON( "version" << version ), file, version );
        auto devices = nma().getDBClient().findAll( "db.addons.cloud_devices.devices", JSON_EMPTY_OBJ, nullptr );
        std::vector< std::string > dumpedDevices;
        std::transform( devices.begin(), devices.end(), std::back_inserter( dumpedDevices ), []( const Json& device ) {
            return device.dump( 4 );
        } );
        std::sort( dumpedDevices.begin(), dumpedDevices.end() );
        return dumpedDevices;
    }
};

#define TEST_WAIT_FOR_NEW_DEVICES_IN_DB( version ) waitForNewDevicesInDb( version, __FILE__, __LINE__ )

TEST_F( CloudDevicesAddonTest, RequestCloudDevicesAfterCloudRegistration ) {  // 1.1

    reconnectToNma();

    auto request = TEST_WAIT_FOR_REQUEST();  // ignoring tcds cloud method response
    request = TEST_WAIT_FOR_REQUEST();

    EXPECT_EQ( request[ "method" ], "cloud.controller_abstracts_list" );
    EXPECT_EQ( request[ "api" ], "1" );
    setCurrentVersion( json_get_int( request[ "params" ], "version" ) );
    sendResponse(
        request,
        PARSE_JSON( R"({ "version": )" + std::to_string( getCurrentVersion() ) + R"(, "not_modified": 1 })" ) );
}

TEST_F( CloudDevicesAddonTest, HandleCloudDevicesReRequestOnError ) {  // 2.1

    sendCloudRegistered();

    {
        auto request = TEST_WAIT_FOR_REQUEST();

        EXPECT_EQ( request[ "method" ], "cloud.controller_abstracts_list" );
        EXPECT_EQ( request[ "api" ], "1" );
        setCurrentVersion( json_get_int( request[ "params" ], "version" ) );
        sendResponse( request,
                      JSON_EMPTY_OBJ,
                      PARSE_JSON( "{\n"
                                  "    \"code\": 11,\n"
                                  "    \"data\": \"cloud.error.method_not_found\",\n"
                                  "    \"description\": \"\"\n"
                                  "}" ) );
    }

    {
        auto request = TEST_WAIT_FOR_REQUEST();

        EXPECT_EQ( request[ "method" ], "cloud.controller_abstracts_list" );
        EXPECT_EQ( request[ "api" ], "1" );
        setCurrentVersion( json_get_int( request[ "params" ], "version" ) );
        sendResponse(
            request,
            PARSE_JSON( R"({ "version": )" + std::to_string( getCurrentVersion() ) + R"(, "not_modified": 1 })" ) );
    }
}

TEST_F( CloudDevicesAddonTest, HandleCloudDevicesReRequestOnTimeout ) {  // 2.2

    sendCloudRegistered();

    {
        auto request = TEST_WAIT_FOR_REQUEST();

        EXPECT_EQ( request[ "method" ], "cloud.controller_abstracts_list" );
        EXPECT_EQ( request[ "api" ], "1" );
        setCurrentVersion( json_get_int( request[ "params" ], "version" ) );

        timeoutResponse();
        sendResponse(
            request,
            PARSE_JSON( R"({ "version": )" + std::to_string( getCurrentVersion() ) + R"(, "not_modified": 1 })" ) );
    }

    {
        auto request = TEST_WAIT_FOR_REQUEST();

        EXPECT_EQ( request[ "method" ], "cloud.controller_abstracts_list" );
        EXPECT_EQ( request[ "api" ], "1" );
        setCurrentVersion( json_get_int( request[ "params" ], "version" ) );
        sendResponse(
            request,
            PARSE_JSON( R"({ "version": )" + std::to_string( getCurrentVersion() ) + R"(, "not_modified": 1 })" ) );
    }
}

TEST_F( CloudDevicesAddonTest, HandleCloudDevicesCorrectResponse ) {  // 2.3

    sendCloudRegistered();

    {
        auto request = TEST_WAIT_FOR_REQUEST();

        EXPECT_EQ( request[ "method" ], "cloud.controller_abstracts_list" );
        setCurrentVersion( json_get_int( request[ "params" ], "version" ) );
        increaseCurrentVersion();
        sendResponse( request,
                      PARSE_JSON( "{\n"
                                  "    \"devices\": {\n"
                                  "        \"9d066d90-5a27-11ea-bf51-adf6f33c4534\": {\n"
                                  "            \"battery_powered\": 0,\n"
                                  "            \"capabilities\": [\n"
                                  "                \"text_command\",\n"
                                  "                \"ui_flag_alexa_account\"\n"
                                  "            ],\n"
                                  "            \"category\": \"assistant\",\n"
                                  "            \"device_type_id\": \"alexa_account\",\n"
                                  "            \"id\": \"9d066d90-5a27-11ea-bf51-adf6f33c4534\",\n"
                                  "            \"info\": {\n"
                                  "                \"account\": \"amazoncorp@ezlo.com\",\n"
                                  "                \"vendor\": \"alexa\"\n"
                                  "            },\n"
                                  "            \"name\": \"92700e57-fb67-4f83-bfaf-09f93f657177\",\n"
                                  "            \"protocol\": {\n"
                                  "                \"vendor\": \"alexa\"\n"
                                  "            },\n"
                                  "            \"reachable\": 1,\n"
                                  "            \"reachable_reason\": \"assistant.voice\",\n"
                                  "            \"security\": \"high\",\n"
                                  "            \"subcategory\": \"voice\",\n"
                                  "            \"type\": \"assistant.voice\"\n"
                                  "        }\n"
                                  "    },\n"
                                  "    \"version\": " +
                                  std::to_string( getCurrentVersion() ) +
                                  "\n"
                                  "}" ) );
    }

    {
        auto devices = TEST_WAIT_FOR_NEW_DEVICES_IN_DB( getCurrentVersion() );
        ASSERT_THAT( devices,
                     ::testing::ElementsAre( "{\n"
                                             "    \"_id\": \"9d066d90-5a27-11ea-bf51-adf6f33c4534\",\n"
                                             "    \"battery_powered\": 0,\n"
                                             "    \"capabilities\": [\n"
                                             "        \"text_command\",\n"
                                             "        \"ui_flag_alexa_account\"\n"
                                             "    ],\n"
                                             "    \"category\": \"assistant\",\n"
                                             "    \"device_type_id\": \"alexa_account\",\n"
                                             "    \"info\": {\n"
                                             "        \"account\": \"amazoncorp@ezlo.com\",\n"
                                             "        \"vendor\": \"alexa\"\n"
                                             "    },\n"
                                             "    \"name\": \"92700e57-fb67-4f83-bfaf-09f93f657177\",\n"
                                             "    \"protocol\": {\n"
                                             "        \"vendor\": \"alexa\"\n"
                                             "    },\n"
                                             "    \"reachable\": 1,\n"
                                             "    \"reachable_reason\": \"assistant.voice\",\n"
                                             "    \"security\": \"high\",\n"
                                             "    \"subcategory\": \"voice\",\n"
                                             "    \"type\": \"assistant.voice\"\n"
                                             "}" ) );
    }
}

TEST_F( CloudDevicesAddonTest, HandleCloudDeviceEventsAddedCorrect ) {  // 3.1.1

    sendCloudRegistered();

    {
        auto request = TEST_WAIT_FOR_REQUEST();

        EXPECT_EQ( request[ "method" ], "cloud.controller_abstracts_list" );
        setCurrentVersion( json_get_int( request[ "params" ], "version" ) );
        increaseCurrentVersion();
        sendResponse( request,
                      PARSE_JSON( "{\n"
                                  "    \"devices\": {\n"
                                  "       \"3.1.1-1\": { \"id\": \"3.1.1-1\" }\n"
                                  "    },\n"
                                  "    \"version\": " +
                                  std::to_string( getCurrentVersion() ) +
                                  "\n"
                                  "}" ) );
    }

    {
        auto devices = TEST_WAIT_FOR_NEW_DEVICES_IN_DB( getCurrentVersion() );
        ASSERT_THAT( devices,
                     ::testing::ElementsAre( "{\n"
                                             "    \"_id\": \"3.1.1-1\"\n"
                                             "}" ) );
    }

    increaseCurrentVersion();
    sendBroadcast( "cloud.abstracts.added",
                   PARSE_JSON( "{\n"
                               "    \"version\": " +
                               std::to_string( getCurrentVersion() ) +
                               ",\n"
                               "    \"devices\": {\n"
                               "       \"3.1.1-2\": { \"id\": \"3.1.1-2\" }\n"
                               "    }\n"
                               "}" ) );

    {
        auto devices = TEST_WAIT_FOR_NEW_DEVICES_IN_DB( getCurrentVersion() );
        ASSERT_THAT( devices,
                     ::testing::ElementsAre( "{\n"
                                             "    \"_id\": \"3.1.1-1\"\n"
                                             "}",
                                             "{\n"
                                             "    \"_id\": \"3.1.1-2\"\n"
                                             "}" ) );
    }
}

TEST_F( CloudDevicesAddonTest, HandleCloudDeviceEventsAddedExistingDevice ) {  // 3.1.2

    sendCloudRegistered();

    {
        auto request = TEST_WAIT_FOR_REQUEST();

        EXPECT_EQ( request[ "method" ], "cloud.controller_abstracts_list" );
        setCurrentVersion( json_get_int( request[ "params" ], "version" ) );
        increaseCurrentVersion();
        sendResponse( request,
                      PARSE_JSON( "{\n"
                                  "    \"devices\": {\n"
                                  "       \"3.1.2-1\": { \"id\": \"3.1.2-1\" }\n"
                                  "    },\n"
                                  "    \"version\": " +
                                  std::to_string( getCurrentVersion() ) +
                                  "\n"
                                  "}" ) );
    }

    {
        auto devices = TEST_WAIT_FOR_NEW_DEVICES_IN_DB( getCurrentVersion() );
        ASSERT_THAT( devices,
                     ::testing::ElementsAre( "{\n"
                                             "    \"_id\": \"3.1.2-1\"\n"
                                             "}" ) );
    }

    increaseCurrentVersion();
    sendBroadcast( "cloud.abstracts.added",
                   PARSE_JSON( "{\n"
                               "    \"version\": " +
                               std::to_string( getCurrentVersion() ) +
                               ",\n"
                               "    \"devices\": {\n"
                               "       \"3.1.2-1\": { \"id\": \"3.1.2-1\" }\n"
                               "    }\n"
                               "}" ) );

    {
        auto request = TEST_WAIT_FOR_REQUEST();

        EXPECT_EQ( request[ "method" ], "cloud.controller_abstracts_list" );
        setCurrentVersion( json_get_int( request[ "params" ], "version" ) );
        sendResponse(
            request,
            PARSE_JSON( "{ \"version\":  " + std::to_string( getCurrentVersion() ) + ", \"not_modified\": 1 }" ) );
    }
}

TEST_F( CloudDevicesAddonTest, HandleCloudDeviceEventsDeletedCorrect ) {  // 3.2.1

    sendCloudRegistered();

    {
        auto request = TEST_WAIT_FOR_REQUEST();

        EXPECT_EQ( request[ "method" ], "cloud.controller_abstracts_list" );
        setCurrentVersion( json_get_int( request[ "params" ], "version" ) );
        increaseCurrentVersion();
        sendResponse( request,
                      PARSE_JSON( "{\n"
                                  "    \"devices\": {\n"
                                  "       \"3.2.1-1\": { \"id\": \"3.2.1-1\" },\n"
                                  "       \"3.2.1-2\": { \"id\": \"3.2.1-2\" }\n"
                                  "    },\n"
                                  "    \"version\": " +
                                  std::to_string( getCurrentVersion() ) +
                                  "\n"
                                  "}" ) );
    }

    {
        auto devices = TEST_WAIT_FOR_NEW_DEVICES_IN_DB( getCurrentVersion() );
        ASSERT_THAT( devices,
                     ::testing::ElementsAre( "{\n"
                                             "    \"_id\": \"3.2.1-1\"\n"
                                             "}",
                                             "{\n"
                                             "    \"_id\": \"3.2.1-2\"\n"
                                             "}" ) );
    }

    increaseCurrentVersion();
    sendBroadcast( "cloud.abstracts.deleted",
                   PARSE_JSON( "{\n"
                               "    \"version\": " +
                               std::to_string( getCurrentVersion() ) +
                               ",\n"
                               "    \"devices\": [ \"3.2.1-1\" ]\n"
                               "}" ) );

    {
        auto devices = TEST_WAIT_FOR_NEW_DEVICES_IN_DB( getCurrentVersion() );
        ASSERT_THAT( devices,
                     ::testing::ElementsAre( "{\n"
                                             "    \"_id\": \"3.2.1-2\"\n"
                                             "}" ) );
    }
}

TEST_F( CloudDevicesAddonTest, HandleCloudDeviceEventsDeletedUnknownDevice ) {  // 3.2.2

    sendCloudRegistered();

    {
        auto request = TEST_WAIT_FOR_REQUEST();

        EXPECT_EQ( request[ "method" ], "cloud.controller_abstracts_list" );
        setCurrentVersion( json_get_int( request[ "params" ], "version" ) );
        increaseCurrentVersion();
        sendResponse( request,
                      PARSE_JSON( "{\n"
                                  "    \"devices\": {\n"
                                  "       \"3.2.2-1\": { \"id\": \"3.2.2-1\" },\n"
                                  "       \"3.2.2-2\": { \"id\": \"3.2.2-2\" }\n"
                                  "    },\n"
                                  "    \"version\": " +
                                  std::to_string( getCurrentVersion() ) +
                                  "\n"
                                  "}" ) );
    }

    {
        auto devices = TEST_WAIT_FOR_NEW_DEVICES_IN_DB( getCurrentVersion() );
        ASSERT_THAT( devices,
                     ::testing::ElementsAre( "{\n"
                                             "    \"_id\": \"3.2.2-1\"\n"
                                             "}",
                                             "{\n"
                                             "    \"_id\": \"3.2.2-2\"\n"
                                             "}" ) );
    }

    increaseCurrentVersion();
    sendBroadcast( "cloud.abstracts.deleted",
                   PARSE_JSON( "{\n"
                               "    \"version\": " +
                               std::to_string( getCurrentVersion() ) +
                               ",\n"
                               "    \"devices\": [ \"3.2.2-3\" ]\n"
                               "}" ) );

    {
        auto request = TEST_WAIT_FOR_REQUEST();

        EXPECT_EQ( request[ "method" ], "cloud.controller_abstracts_list" );
        setCurrentVersion( json_get_int( request[ "params" ], "version" ) );
        sendResponse(
            request,
            PARSE_JSON( "{ \"version\":  " + std::to_string( getCurrentVersion() ) + ", \"not_modified\": 1 }" ) );
    }
}

TEST_F( CloudDevicesAddonTest, HandleCloudDeviceEventsUpdatedCorrect ) {  // 3.3.1

    sendCloudRegistered();

    {
        auto request = TEST_WAIT_FOR_REQUEST();

        EXPECT_EQ( request[ "method" ], "cloud.controller_abstracts_list" );
        setCurrentVersion( json_get_int( request[ "params" ], "version" ) );
        increaseCurrentVersion();
        sendResponse( request,
                      PARSE_JSON( "{\n"
                                  "    \"devices\": {\n"
                                  "        \"3.3.1-1\": {\n"
                                  "            \"id\": \"3.3.1-1\",\n"
                                  "            \"boolKey\": true,\n"
                                  "            \"intKey\": 0,\n"
                                  "            \"floatKey\": 0.25,\n"
                                  "            \"stringKey\": \"stringVal\",\n"
                                  "            \"arrayKey\": [ \"item1\", \"item2\" ],\n"
                                  "            \"objectKey\": {\n"
                                  "                \"subKey1\": \"subValue1\",\n"
                                  "                \"subKey2\": \"subValue2\"\n"
                                  "            },\n"
                                  "            \"persistentKey\": \"persistentVal\"\n"
                                  "        }\n"
                                  "    },\n"
                                  "    \"version\": " +
                                  std::to_string( getCurrentVersion() ) +
                                  "\n"
                                  "}" ) );
    }

    {
        auto devices = TEST_WAIT_FOR_NEW_DEVICES_IN_DB( getCurrentVersion() );
        EXPECT_EQ( devices.size(), 1 );
        EXPECT_EQ( "{\n"
                   "    \"_id\": \"3.3.1-1\",\n"
                   "    \"arrayKey\": [\n"
                   "        \"item1\",\n"
                   "        \"item2\"\n"
                   "    ],\n"
                   "    \"boolKey\": true,\n"
                   "    \"floatKey\": 0.25,\n"
                   "    \"intKey\": 0,\n"
                   "    \"objectKey\": {\n"
                   "        \"subKey1\": \"subValue1\",\n"
                   "        \"subKey2\": \"subValue2\"\n"
                   "    },\n"
                   "    \"persistentKey\": \"persistentVal\",\n"
                   "    \"stringKey\": \"stringVal\"\n"
                   "}",
                   devices.front() );
    }

    increaseCurrentVersion();
    sendBroadcast( "cloud.abstracts.updated",
                   PARSE_JSON( "{\n"
                               "    \"version\": " +
                               std::to_string( getCurrentVersion() ) +
                               ",\n"
                               "    \"devices\": {\n"
                               "        \"3.3.1-1\": {\n"
                               "            \"boolKey\": false,\n"
                               "            \"intKey\": 42,\n"
                               "            \"floatKey\": 2.75,\n"
                               "            \"stringKey\": \"newStringVal\",\n"
                               "            \"arrayKey\": [ \"item2\", \"item3\" ],\n"
                               "            \"objectKey\": {\n"
                               "                \"subKey2\": \"subValue2\",\n"
                               "                \"subKey3\": \"subValue3\"\n"
                               "            }\n"
                               "        }\n"
                               "    }\n"
                               "}" ) );

    {
        auto devices = TEST_WAIT_FOR_NEW_DEVICES_IN_DB( getCurrentVersion() );
        EXPECT_EQ( devices.size(), 1 );
        EXPECT_EQ( "{\n"
                   "    \"_id\": \"3.3.1-1\",\n"
                   "    \"arrayKey\": [\n"
                   "        \"item2\",\n"
                   "        \"item3\"\n"
                   "    ],\n"
                   "    \"boolKey\": false,\n"
                   "    \"floatKey\": 2.75,\n"
                   "    \"intKey\": 42,\n"
                   "    \"objectKey\": {\n"
                   "        \"subKey2\": \"subValue2\",\n"
                   "        \"subKey3\": \"subValue3\"\n"
                   "    },\n"
                   "    \"persistentKey\": \"persistentVal\",\n"
                   "    \"stringKey\": \"newStringVal\"\n"
                   "}",
                   devices.front() );
    }
}

TEST_F( CloudDevicesAddonTest, HandleCloudDeviceEventsUpdatedUnknownDevice ) {  // 3.3.2

    sendCloudRegistered();

    {
        auto request = TEST_WAIT_FOR_REQUEST();

        EXPECT_EQ( request[ "method" ], "cloud.controller_abstracts_list" );
        setCurrentVersion( json_get_int( request[ "params" ], "version" ) );
        increaseCurrentVersion();
        sendResponse( request,
                      PARSE_JSON( "{\n"
                                  "    \"devices\": {\n"
                                  "        \"3.3.2-1\": {\n"
                                  "            \"id\": \"3.3.2-1\",\n"
                                  "            \"key\": \"value1\"\n"
                                  "        }\n"
                                  "    },\n"
                                  "    \"version\": " +
                                  std::to_string( getCurrentVersion() ) +
                                  "\n"
                                  "}" ) );
    }

    {
        auto devices = TEST_WAIT_FOR_NEW_DEVICES_IN_DB( getCurrentVersion() );
        EXPECT_EQ( devices.size(), 1 );
        EXPECT_EQ( "{\n"
                   "    \"_id\": \"3.3.2-1\",\n"
                   "    \"key\": \"value1\"\n"
                   "}",
                   devices.front() );
    }

    increaseCurrentVersion();
    sendBroadcast( "cloud.abstracts.updated",
                   PARSE_JSON( "{\n"
                               "    \"version\": " +
                               std::to_string( getCurrentVersion() ) +
                               ",\n"
                               "    \"devices\": {\n"
                               "        \"3.3.2-2\": {\n"
                               "            \"key\": \"value2\"\n"
                               "        }\n"
                               "    }\n"
                               "}" ) );

    {
        auto request = TEST_WAIT_FOR_REQUEST();

        EXPECT_EQ( request[ "method" ], "cloud.controller_abstracts_list" );
        setCurrentVersion( json_get_int( request[ "params" ], "version" ) );
        sendResponse(
            request,
            PARSE_JSON( "{ \"version\":  " + std::to_string( getCurrentVersion() ) + ", \"not_modified\": 1 }" ) );
    }
}

TEST_F( CloudDevicesAddonTest, HandleCloudDeviceEventsWithoutVersion ) {  // 3.4

    sendCloudRegistered();

    {
        auto request = TEST_WAIT_FOR_REQUEST();

        EXPECT_EQ( request[ "method" ], "cloud.controller_abstracts_list" );
        setCurrentVersion( json_get_int( request[ "params" ], "version" ) );
        increaseCurrentVersion();
        sendResponse( request,
                      PARSE_JSON( "{\n"
                                  "    \"devices\": {\n"
                                  "       \"3.4-1\": { \"id\": \"3.4-1\" }\n"
                                  "    },\n"
                                  "    \"version\": " +
                                  std::to_string( getCurrentVersion() ) +
                                  "\n"
                                  "}" ) );
    }

    {
        auto devices = TEST_WAIT_FOR_NEW_DEVICES_IN_DB( getCurrentVersion() );
        ASSERT_THAT( devices,
                     ::testing::ElementsAre( "{\n"
                                             "    \"_id\": \"3.4-1\"\n"
                                             "}" ) );
    }

    increaseCurrentVersion();
    sendBroadcast( "cloud.abstracts.added",
                   PARSE_JSON( "{\n"
                               "    \"devices\": {\n"
                               "       \"3.4-2\": { \"id\": \"3.4-2\" }\n"
                               "    }\n"
                               "}" ) );

    {
        auto request = TEST_WAIT_FOR_REQUEST();

        EXPECT_EQ( request[ "method" ], "cloud.controller_abstracts_list" );
        setCurrentVersion( json_get_int( request[ "params" ], "version" ) );
        sendResponse(
            request,
            PARSE_JSON( "{ \"version\":  " + std::to_string( getCurrentVersion() ) + ", \"not_modified\": 1 }" ) );
    }
}

TEST_F( CloudDevicesAddonTest, HandleCloudDeviceEventsRequestWithVersionThatDiffersFromCcurrentNotByOne ) {  // 3.5

    sendCloudRegistered();

    {
        auto request = TEST_WAIT_FOR_REQUEST();

        EXPECT_EQ( request[ "method" ], "cloud.controller_abstracts_list" );
        setCurrentVersion( json_get_int( request[ "params" ], "version" ) );
        sendResponse(
            request,
            PARSE_JSON( "{ \"version\":  " + std::to_string( getCurrentVersion() ) + ", \"not_modified\": 1 }" ) );
    }

    increaseCurrentVersion();
    increaseCurrentVersion();
    sendBroadcast( "cloud.abstracts.added",
                   PARSE_JSON( "{\n"
                               "    \"version\": " +
                               std::to_string( getCurrentVersion() ) +
                               ",\n"
                               "    \"devices\": {\n"
                               "       \"3.5-1\": { \"id\": \"3.5-1\" }\n"
                               "    }\n"
                               "}" ) );

    {
        auto request = TEST_WAIT_FOR_REQUEST();

        EXPECT_EQ( request[ "method" ], "cloud.controller_abstracts_list" );
        sendResponse( request,
                      PARSE_JSON( "{\n"
                                  "    \"devices\": {\n"
                                  "        \"3.5-2\": { \"id\": \"3.5-2\" },\n"
                                  "        \"3.5-3\": { \"id\": \"3.5-3\" }\n"
                                  "    },\n"
                                  "    \"version\": " +
                                  std::to_string( getCurrentVersion() ) +
                                  "\n"
                                  "}" ) );
    }
}

}  // namespace Tests
}  // namespace HubPlatform
