#include "common_test_resources/fixtures/scene_manager_tests_fixture.h"
#include "common_test_resources/resources/scene_manager_resources.h"
#include "common_test_resources/resources/cored_resources.h"
#include "cored/house_modes/house_mode_utils.h"

#include "cored/model/values/value_factory.h"
#include <gtest/gtest.h>
#include "tiny/tiny_consolecolors.h"

namespace HubPlatform {
namespace Tests {

/*

TESTPLAN

DONE    1 Create device group
DONE        1.1 Create device group Pass
DONE        1.2 Create device group Fail
DONE    2 Delete device group
DONE        2.1 Delete device group
DONE        2.2 Delete persistent device group error
DONE        2.3 Delete persistent device group with 'force' flag
DONE        2.4 Delete device group error
DONE    3 Update device group
DONE        3.1 Update device group
DONE        3.2 Update device group error
DONE        3.3 Update device group error with wrong entryDelay
DONE        3.4 Update device group error with role field
DONE    4 Device groups list
DONE        4.1 Device groups list success
DONE        4.2 Device groups list version success
DONE    5 Device groups list
DONE        5.1 Device groups list success
DONE        5.2 Device groups list error
DONE   6 Device removing test
DONE        6.1 Remove device which include in group['device']
DONE        6.2 Remove device which not include in group['device']
DONE        6.3 Remove device which include in group['exceptions']
DONE        7.1 Device groups find success
DONE        7.2 Device groups find error
DONE   8 Device groups group delay
DONE        8.1 Device groups group 'instant' delay
DONE   9 Predefine Device Group list
DONE        9.1 Success
*/

class SceneDeviceGroupTests : public SceneManagerTestsFixture {
public:
    static void SetUpTestCase();
    void SetUp() override;
    void TearDown() override;

    void removeDevice( const std::string& deviceId, int codeLine );
    void checkDevicesAndExceptions( const std::string id, bool isExistDevices, bool isExistExceptions, int codeLine );
    void deleteDeviceGroup( const std::string& id, int codeLine );
};

void SceneDeviceGroupTests::SetUpTestCase() {}

void SceneDeviceGroupTests::SetUp() {
    SceneManagerTestsFixture::SetUp();
    callingFile( __FILE__ );
}

void SceneDeviceGroupTests::TearDown() {
    auto ignoreAllPresentBroadcasts = []( const JsonObject& br ) {
        Log::error( cc::error( "# SceneDeviceGroupTests::TearDown: Not handled broadcasts: " ) + cc::j( br ) );
        return true;
    };
    ignoreBroadcasts( ignoreAllPresentBroadcasts, __FILE__, __LINE__, std::chrono::milliseconds( 100 ) );

    SceneManagerTestsFixture::TearDown();
}

void SceneDeviceGroupTests::removeDevice( const std::string& deviceId, int codeLine ) {
    auto& remoteInterface = makeRemote( nma() );
    auto itemsNumber = remoteInterface.getItemsByDeviceId( deviceId ).size();
    CoreNmaTest::removeDevice( remoteInterface, deviceId, itemsNumber, 0, codeLine );
}

void SceneDeviceGroupTests::checkDevicesAndExceptions( const std::string id,
                                                       bool isExistDevices,
                                                       bool isExistExceptions,
                                                       int codeLine ) {
    std::string codeLineStr = "code line: " + std::to_string( codeLine );
    auto answer = nma().sendtoHubAndGetAnswer( Json::parse( deviceGroupsList ) );
    auto error = answer[ "error" ];
    EXPECT_TRUE( error.empty() );
    auto result = answer[ "result" ];
    auto deviceGroups = result[ "deviceGroups" ];
    bool foundGroup = false;
    for( auto group : deviceGroups ) {
        if( group[ "_id" ] == id ) {
            foundGroup = true;
            EXPECT_EQ( !group[ "devices" ].empty(), isExistDevices ) << codeLineStr;
            EXPECT_EQ( !group[ "exceptions" ].empty(), isExistExceptions ) << codeLineStr;
        }
    }

    EXPECT_TRUE( foundGroup ) << codeLineStr;
}

void SceneDeviceGroupTests::deleteDeviceGroup( const std::string& groupId, int codeLine ) {
    std::string codeLineStr = "code line: " + std::to_string( codeLine );
    auto request =
        replaceTokensInString( scenesDeviceGroupDelete, { { "__GROUP_ID__", groupId }, { "__FORCE__", "false" } } );
    sendRequestAndCheckAnswer( Json::parse( request ), __LINE__ );
    auto broadcast = waitForBroadcast( "hub.device.group.deleted", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.group.deleted" ) << codeLineStr;
}
TEST_F( SceneDeviceGroupTests, createDeviceGroup ) {  // 1.1
    auto request =
        replaceTokensInString( scenesDeviceGroupCreate, { { "_DEVICE_GROUP_NAME_", "testCreateDeviceGroup" } } );

    sendRequestAndCheckAnswer( Json::parse( request ), __LINE__ );
    JsonObject broadcast = waitForBroadcast( "hub.device.group.created", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.group.created" );
    JsonObject result;
    json_get( broadcast, "result", result );
    EXPECT_TRUE( json_contains( result, "entryDelay" ) );
    EXPECT_TRUE( json_contains( result, "followEntry" ) );
    EXPECT_TRUE( json_contains( result, "role" ) );
    EXPECT_TRUE( json_contains( result, "packageId" ) );
    auto id = json_get_str( broadcast[ "result" ], "_id" );
    deleteDeviceGroup( id, __LINE__ );
}

TEST_F( SceneDeviceGroupTests, createDeviceGroupFail ) {  // 1.2
    {
        auto request =
            replaceTokensInString( scenesDeviceGroupCreate, { { "_DEVICE_GROUP_NAME_", "testCreateDeviceGroup" } } );

        auto request2 = Json::parse( request );
        request2[ "params" ][ "packageId" ] = "123";
        auto answer = sendRequestAndCheckFailedAnswer( request2, __LINE__ );
        const auto& error = answer[ "error" ];
        EXPECT_FALSE( error.empty() );
        EXPECT_EQ( error[ "field" ], "packageId" );
        EXPECT_EQ( error[ "message" ], "Object not found" );
        EXPECT_EQ( error[ "reason" ], "package id should be empty" );
    }

    {
        auto request =
            replaceTokensInString( scenesDeviceGroupCreate, { { "_DEVICE_GROUP_NAME_", "testCreateDeviceGroup2" } } );

        auto request2 = Json::parse( request );
        request2[ "params" ].erase( "entryDelay" );
        request2[ "params" ].erase( "role" );
        request2[ "params" ].erase( "packageId" );
        sendRequestAndCheckAnswer( Json::parse( request ), __LINE__ );
        JsonObject broadcast = waitForBroadcast( "hub.device.group.created", getCodeLineString( __LINE__ ) );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.group.created" );
        JsonObject result;
        json_get( broadcast, "result", result );
        EXPECT_TRUE( json_contains( result, "entryDelay" ) );
    }
}

TEST_F( SceneDeviceGroupTests, deleteDeviceGroup ) {  // 2.1
    auto request = replaceTokensInString( scenesDeviceGroupCreate, { { "_DEVICE_GROUP_NAME_", "nameDeleted" } } );
    sendRequestAndCheckAnswer( Json::parse( request ), __LINE__ );
    JsonObject broadcast = waitForBroadcast( "hub.device.group.created", getCodeLineString( __LINE__ ) );
    auto id = json_get_str( broadcast[ "result" ], "_id" );
    deleteDeviceGroup( id, __LINE__ );
}

TEST_F( SceneDeviceGroupTests, deletePersistentDeviceGroup ) {  // 2.2
    auto request = replaceTokensInString( scenesPersistentDeviceGroupCreate,
                                          { { "_DEVICE_GROUP_NAME_", "nameDeleted1" }, { "__PERSISTENT__", "true" } } );
    sendRequestAndCheckAnswer( Json::parse( request ), __LINE__ );
    JsonObject broadcast = waitForBroadcast( "hub.device.group.created", getCodeLineString( __LINE__ ) );
    auto id = json_get_str( broadcast[ "result" ], "_id" );
    request = replaceTokensInString( scenesDeviceGroupDelete, { { "__GROUP_ID__", id }, { "__FORCE__", "false" } } );
    auto answer = nma().sendtoHubAndGetAnswer( Json::parse( request ) );
    const auto& error = answer[ "error" ];
    EXPECT_FALSE( error.empty() );
    EXPECT_EQ( json_get_int( error, "code" ), -32602 );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.delete.group.impossible" );
    EXPECT_EQ( json_get_str( error, "message" ), "Can't delete persistent group" );
}

TEST_F( SceneDeviceGroupTests, deleteForcePersistentDeviceGroup ) {  // 2.3
    auto request = replaceTokensInString( scenesPersistentDeviceGroupCreate,
                                          { { "_DEVICE_GROUP_NAME_", "nameDeleted2" }, { "__PERSISTENT__", "true" } } );
    sendRequestAndCheckAnswer( Json::parse( request ), __LINE__ );
    JsonObject broadcast = waitForBroadcast( "hub.device.group.created", getCodeLineString( __LINE__ ) );
    auto id = json_get_str( broadcast[ "result" ], "_id" );
    request = replaceTokensInString( scenesDeviceGroupDelete, { { "__GROUP_ID__", id }, { "__FORCE__", "true" } } );
    sendRequestAndCheckAnswer( Json::parse( request ), __LINE__ );
    broadcast = waitForBroadcast( "hub.device.group.deleted", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.group.deleted" );
}

TEST_F( SceneDeviceGroupTests, deleteDeviceGroupError ) {  // 2.4
    auto request =
        replaceTokensInString( scenesDeviceGroupDelete, { { "__GROUP_ID__", "badId" }, { "__FORCE__", "false" } } );

    auto answer = nma().sendtoHubAndGetAnswer( Json::parse( request ) );
    const auto& error = answer[ "error" ];
    EXPECT_FALSE( error.empty() );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.not.exist" );
    EXPECT_EQ( json_get_str( error, "field" ), "_id" );
}

TEST_F( SceneDeviceGroupTests, updateDeviceGroup ) {  // 3.1
    auto request = replaceTokensInString( scenesDeviceGroupCreate, { { "_DEVICE_GROUP_NAME_", "name2" } } );
    sendRequestAndCheckAnswer( Json::parse( request ), __LINE__ );
    JsonObject broadcast = waitForBroadcast( "hub.device.group.created", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.group.created" );
    auto deviceGroupId = json_get_str( broadcast[ "result" ], "_id" );
    request = replaceTokensInString( scenesDeviceGroupUpdate, { { "__GROUP_ID__", deviceGroupId } } );
    sendRequestAndCheckAnswer( Json::parse( request ), __LINE__ );
    broadcast = waitForBroadcast( "hub.device.group.updated", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.group.updated" );
    auto id = json_get_str( broadcast[ "result" ], "_id" );
    deleteDeviceGroup( id, __LINE__ );
}

TEST_F( SceneDeviceGroupTests, updateGroupError ) {  // 3.2
    auto request = replaceTokensInString( scenesDeviceGroupUpdate, { { "__GROUP_ID__", "badId" } } );
    auto answer = nma().sendtoHubAndGetAnswer( Json::parse( request ) );
    const auto& error = answer[ "error" ];
    EXPECT_FALSE( error.empty() );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.not.exist" );
    EXPECT_EQ( json_get_str( error, "field" ), "_id" );
}

TEST_F( SceneDeviceGroupTests, updateGroupErrorEntryDelay ) {  // 3.3

    auto request = replaceTokensInString( scenesDeviceGroupCreate, { { "_DEVICE_GROUP_NAME_", "name2.1" } } );
    sendRequestAndCheckAnswer( Json::parse( request ), __LINE__ );
    JsonObject broadcast = waitForBroadcast( "hub.device.group.created", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.group.created" );
    auto deviceGroupId = json_get_str( broadcast[ "result" ], "_id" );

    request = replaceTokensInString( scenesDeviceGroupUpdate, { { "__GROUP_ID__", deviceGroupId } } );
    auto newRequest = Json::parse( request );
    newRequest[ "params" ][ "entryDelay" ] = "1423";

    auto answer = nma().sendtoHubAndGetAnswer( newRequest );
    const auto& error = answer[ "error" ];
    EXPECT_FALSE( error.empty() );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.invalid" );
    EXPECT_EQ( json_get_str( error, "field" ), "entryDelay" );
    EXPECT_EQ( json_get_str( error, "reason" ), "Invalid value for entryDelay enum" );
    deleteDeviceGroup( deviceGroupId, __LINE__ );
}

TEST_F( SceneDeviceGroupTests, updateGroupErrorRole ) {  // 3.4

    auto request = replaceTokensInString( scenesDeviceGroupCreate, { { "_DEVICE_GROUP_NAME_", "name2.2" } } );
    sendRequestAndCheckAnswer( Json::parse( request ), __LINE__ );
    JsonObject broadcast = waitForBroadcast( "hub.device.group.created", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.group.created" );
    auto deviceGroupId = json_get_str( broadcast[ "result" ], "_id" );

    request = replaceTokensInString( scenesDeviceGroupUpdate, { { "__GROUP_ID__", deviceGroupId } } );

    auto jsonRequest = Json::parse( request );
    jsonRequest[ "params" ][ "role" ] = "house_modes";
    auto answer = nma().sendtoHubAndGetAnswer( jsonRequest );
    const auto& error = answer[ "error" ];
    EXPECT_FALSE( error.empty() );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.read_only" );
    EXPECT_EQ( json_get_str( error, "field" ), "role" );
    EXPECT_EQ( json_get_str( error, "reason" ), "Read only field, can't be changed" );
    deleteDeviceGroup( deviceGroupId, __LINE__ );
}

TEST_F( SceneDeviceGroupTests, deviceGroupsListTest ) {  // 4.1
    auto request = replaceTokensInString( scenesDeviceGroupCreate, { { "_DEVICE_GROUP_NAME_", "name3" } } );
    sendRequestAndCheckAnswer( Json::parse( request ), __LINE__ );
    JsonObject broadcast = waitForBroadcast( "hub.device.group.created", getCodeLineString( __LINE__ ) );
    auto deviceGroupId = json_get_str( broadcast[ "result" ], "_id" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.group.created" );
    auto answer = nma().sendtoHubAndGetAnswer( Json::parse( deviceGroupsList ) );
    const auto& error = answer[ "error" ];
    EXPECT_TRUE( error.empty() );
    const auto& deviceGroups = answer[ "result" ][ "deviceGroups" ];
    EXPECT_FALSE( deviceGroups.empty() );
    bool idExist = false;
    for( auto group : deviceGroups ) {
        EXPECT_FALSE( group.empty() );
        const auto id = json_get_str( group, "_id" );
        if( id == deviceGroupId ) {
            idExist = true;
            EXPECT_EQ( json_get_str( group, "name" ), "name3" );
            JsonArray categories;
            json_get_arr( group, "categories", categories );
            EXPECT_EQ(
                categories.dump(),
                R"([{"category":"security_sensor","subcategories":["co2","leak"]},{"category":"scene_controller","exceptions":["panic"]}])" );
            ASSERT_TRUE( json_get_arr( group, "devices", categories ) );
            ASSERT_TRUE( json_get_arr( group, "exceptions", categories ) );
            ASSERT_TRUE( json_contains( group, "entryDelay" ) );
            ASSERT_TRUE( json_contains( group, "followEntry" ) );
            ASSERT_TRUE( json_contains( group, "role" ) );
            ASSERT_TRUE( json_contains( group, "packageId" ) );
        }
    }

    EXPECT_TRUE( idExist );
    deleteDeviceGroup( deviceGroupId, __LINE__ );
}

TEST_F( SceneDeviceGroupTests, deviceGroupsListVersionTest ) {  // 4.2
    auto request = replaceTokensInString( scenesDeviceGroupCreate, { { "_DEVICE_GROUP_NAME_", "name4" } } );
    sendRequestAndCheckAnswer( Json::parse( request ), __LINE__ );
    JsonObject broadcast = waitForBroadcast( "hub.device.group.created", getCodeLineString( __LINE__ ) );
    auto deviceGroupId = json_get_str( broadcast[ "result" ], "_id" );
    std::string version;
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.group.created" );
    {
        auto answer = nma().sendtoHubAndGetAnswer( Json::parse( deviceGroupsList ) );
        auto& error = answer[ "error" ];
        EXPECT_TRUE( error.empty() );
        auto& deviceGroups = answer[ "result" ][ "deviceGroups" ];
        version = json_get_str( answer[ "result" ], "version" );
        EXPECT_FALSE( deviceGroups.empty() );
        bool idExist = false;
        for( auto group : deviceGroups ) {
            EXPECT_FALSE( group.empty() );
            const auto id = json_get_str( group, "_id" );
            if( id == deviceGroupId ) {
                idExist = true;
                EXPECT_EQ( json_get_str( group, "name" ), "name4" );
                JsonArray categories;
                json_get_arr( group, "categories", categories );
                EXPECT_EQ(
                    categories.dump(),
                    R"([{"category":"security_sensor","subcategories":["co2","leak"]},{"category":"scene_controller","exceptions":["panic"]}])" );
                ASSERT_TRUE( json_get_arr( group, "devices", categories ) );
                ASSERT_TRUE( json_get_arr( group, "exceptions", categories ) );
            }
        }

        EXPECT_TRUE( idExist );
    }

    {
        request = replaceTokensInString( deviceGroupsList, { { "__VERSION__", version } } );
        auto answer = nma().sendtoHubAndGetAnswer( Json::parse( request ) );
        auto error = answer[ "error" ];
        EXPECT_TRUE( error.empty() );
        auto& deviceGroups = answer[ "result" ][ "deviceGroups" ];
        EXPECT_TRUE( deviceGroups.empty() );
        EXPECT_EQ( answer[ "result" ][ "version" ], version );
    }
    deleteDeviceGroup( deviceGroupId, __LINE__ );
}

TEST_F( SceneDeviceGroupTests, deviceGroupGetTest ) {  // 5.1
    auto request = replaceTokensInString( scenesDeviceGroupCreate, { { "_DEVICE_GROUP_NAME_", "name5" } } );
    sendRequestAndCheckAnswer( Json::parse( request ), __LINE__ );
    JsonObject broadcast = waitForBroadcast( "hub.device.group.created", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.group.created" );
    auto deviceGroupId = json_get_str( broadcast[ "result" ], "_id" );

    request = replaceTokensInString( deviceGroupGet, { { "__GROUP_ID__", deviceGroupId } } );
    auto answer = nma().sendtoHubAndGetAnswer( Json::parse( request ) );
    const auto& error = answer[ "error" ];
    EXPECT_TRUE( error.empty() );
    const auto& result = answer[ "result" ];
    EXPECT_FALSE( result.empty() );
    EXPECT_EQ( json_get_str( result, "_id" ), deviceGroupId );
    EXPECT_EQ( json_get_str( result, "name" ), "name5" );
    JsonArray categories;
    json_get_arr( result, "categories", categories );
    EXPECT_EQ(
        categories.dump(),
        R"([{"category":"security_sensor","subcategories":["co2","leak"]},{"category":"scene_controller","exceptions":["panic"]}])" );
    json_get_arr( result, "devices", categories );
    json_get_arr( result, "exceptions", categories );
    ASSERT_TRUE( json_contains( result, "entryDelay" ) );
    ASSERT_TRUE( json_contains( result, "followEntry" ) );
    ASSERT_TRUE( json_contains( result, "role" ) );
    ASSERT_TRUE( json_contains( result, "packageId" ) );
}

TEST_F( SceneDeviceGroupTests, deviceGroupGetErrorTest ) {  // 5.2
    auto request = replaceTokensInString( scenesDeviceGroupCreate, { { "_DEVICE_GROUP_NAME_", "name6" } } );
    sendRequestAndCheckAnswer( Json::parse( request ), __LINE__ );
    JsonObject broadcast = waitForBroadcast( "hub.device.group.created", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.group.created" );
    auto deviceGroupId = json_get_str( broadcast[ "result" ], "_id" );

    request = replaceTokensInString( deviceGroupGet, { { "__GROUP_ID__", "badId" } } );
    auto answer = nma().sendtoHubAndGetAnswer( Json::parse( request ) );
    const auto& error = answer[ "error" ];
    EXPECT_FALSE( error.empty() );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.not.exist" );
    EXPECT_EQ( json_get_str( error, "field" ), "_id" );
    EXPECT_EQ( json_get_str( error, "message" ), "Record with the key does not exist" );
}

TEST_F( SceneDeviceGroupTests, checkDeviceGroupAfterDeviceRemoving ) {  // 6.1
    using namespace CoreD::Model::Resources;
    auto deviceInfo = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );
    auto deviceId = deviceInfo.m_deviceId;
    std::string devices;
    devices += "[\"" + deviceId + "\"]";

    auto request = replaceTokensInString(
        scenesDeviceGroupCreateWithDevices,
        { { "_DEVICE_GROUP_NAME_", "DeviceRemoving" }, { "__DEVICES__", devices }, { "__EXCEPTIONS__", "[]" } } );
    sendRequestAndCheckAnswer( Json::parse( request ), __LINE__ );
    JsonObject broadcast = waitForBroadcast( "hub.device.group.created", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.group.created" );

    removeDevice( deviceInfo.m_deviceId, __LINE__ );
    broadcast = waitForBroadcast( "hub.device.group.updated", getCodeLineString( __LINE__ ) );
    auto id = json_get_str( broadcast[ "result" ], "_id" );
    const auto& result = broadcast[ "result" ];
    EXPECT_FALSE( result.empty() );

    auto broadcastName = json_get_str( broadcast[ "result" ], "name" );
    JsonArray broadcastDevicesArray;
    json_get_arr( broadcast[ "result" ], "devices", broadcastDevicesArray );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.group.updated" );
    EXPECT_TRUE( broadcastDevicesArray.empty() );
}

TEST_F( SceneDeviceGroupTests, checkDeviceGroupAfterDeleteAnotherDevice ) {  // 6.2
    using namespace CoreD::Model::Resources;
    auto deviceInfo = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );
    auto deviceId = deviceInfo.m_deviceId;
    std::string devices;
    devices += "[\"" + deviceId + "\"]";

    auto request = replaceTokensInString(
        scenesDeviceGroupCreateWithDevices,
        { { "DeviceNotRemoving", "DeviceNotRemoving" }, { "__DEVICES__", devices }, { "__EXCEPTIONS__", "[]" } } );
    sendRequestAndCheckAnswer( Json::parse( request ), __LINE__ );
    JsonObject broadcast = waitForBroadcast( "hub.device.group.created", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.group.created" );
    auto deviceGroupId = json_get_str( broadcast[ "result" ], "_id" );
    auto deviceInfo1 = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );
    removeDevice( deviceInfo1.m_deviceId, __LINE__ );

    request = replaceTokensInString( deviceGroupGet, { { "__GROUP_ID__", deviceGroupId } } );
    auto answer = nma().sendtoHubAndGetAnswer( Json::parse( request ) );
    const auto& error = answer[ "error" ];
    EXPECT_TRUE( error.empty() );
    const auto& result = answer[ "result" ];
    EXPECT_FALSE( result.empty() );
    JsonArray devicesArray;
    json_get_arr( result, "devices", devicesArray );
    EXPECT_EQ( devicesArray.size(), 1 );

    removeDevice( deviceInfo.m_deviceId, __LINE__ );
    broadcast = waitForBroadcast( "hub.device.group.updated", getCodeLineString( __LINE__ ) );
    EXPECT_FALSE( broadcast[ "result" ].empty() );
}

TEST_F( SceneDeviceGroupTests, checkDeviceGroupAfterExceptionRemoving ) {  // 6.3
    using namespace CoreD::Model::Resources;
    auto deviceInfo = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );
    std::string devices;
    devices += "[\"" + deviceInfo.m_deviceId + "\"]";
    auto exceptionDeviceInfo = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );
    std::string exceptions;
    exceptions += "[\"" + exceptionDeviceInfo.m_deviceId + "\"]";

    auto request = replaceTokensInString( scenesDeviceGroupCreateWithDevices,
                                          { { "_DEVICE_GROUP_NAME_", "ExceptionRemoving" },
                                            { "__DEVICES__", devices },
                                            { "__EXCEPTIONS__", exceptions } } );
    sendRequestAndCheckAnswer( Json::parse( request ), __LINE__ );
    JsonObject broadcast = waitForBroadcast( "hub.device.group.created", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.group.created" );
    auto id = json_get_str( broadcast[ "result" ], "_id" );
    removeDevice( exceptionDeviceInfo.m_deviceId, __LINE__ );
    broadcast = waitForBroadcast( "hub.device.group.updated", getCodeLineString( __LINE__ ) );
    const auto& result = broadcast[ "result" ];
    EXPECT_FALSE( result.empty() );

    auto broadcastName = json_get_str( broadcast[ "result" ], "name" );
    JsonArray broadcastDevicesArray;
    json_get_arr( broadcast[ "result" ], "exceptions", broadcastDevicesArray );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.group.updated" );
    EXPECT_TRUE( broadcastDevicesArray.empty() );

    removeDevice( deviceInfo.m_deviceId, __LINE__ );
    broadcast = waitForBroadcast( "hub.device.group.updated", getCodeLineString( __LINE__ ) );
    EXPECT_FALSE( broadcast[ "result" ].empty() );
    checkDevicesAndExceptions( id, false, false, __LINE__ );
    EXPECT_FALSE( broadcast[ "result" ].empty() );
    deleteDeviceGroup( id, __LINE__ );
}

TEST_F( SceneDeviceGroupTests, deviceGroupFindTest ) {  // 7.1
    auto request = replaceTokensInString( scenesDeviceGroupCreate, { { "_DEVICE_GROUP_NAME_", "name7" } } );
    sendRequestAndCheckAnswer( Json::parse( request ), __LINE__ );
    JsonObject broadcast = waitForBroadcast( "hub.device.group.created", getCodeLineString( __LINE__ ) );
    auto id = json_get_str( broadcast[ "result" ], "_id" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.group.created" );
    auto deviceGroupId = json_get_str( broadcast[ "result" ], "_id" );

    request = replaceTokensInString( deviceGroupFind, { { "__GROUP_ID__", deviceGroupId } } );
    auto answer = nma().sendtoHubAndGetAnswer( Json::parse( request ) );
    const auto& error = answer[ "error" ];
    EXPECT_TRUE( error.empty() );
    const auto& result = answer[ "result" ];
    EXPECT_FALSE( result.empty() );
    const auto& deviceGroups = result[ "deviceGroups" ].front();
    EXPECT_EQ( json_get_str( deviceGroups, "id" ), deviceGroupId );
    EXPECT_EQ( json_get_str( deviceGroups, "name" ), "name7" );
    deleteDeviceGroup( id, __LINE__ );
}

TEST_F( SceneDeviceGroupTests, deviceGroupFindErrorTest ) {  // 7.2
    auto request = replaceTokensInString( scenesDeviceGroupCreate, { { "_DEVICE_GROUP_NAME_", "name8" } } );
    sendRequestAndCheckAnswer( Json::parse( request ), __LINE__ );
    JsonObject broadcast = waitForBroadcast( "hub.device.group.created", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.group.created" );
    auto deviceGroupId = json_get_str( broadcast[ "result" ], "_id" );

    request = replaceTokensInString( deviceGroupFind, { { "__GROUP_ID__", "" } } );
    auto answer = nma().sendtoHubAndGetAnswer( Json::parse( request ) );
    const auto& error = answer[ "error" ];
    EXPECT_FALSE( error.empty() );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.invalid" );
    EXPECT_EQ( json_get_str( error, "field" ), "deviceGroupId" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong params" );
    EXPECT_EQ( json_get_str( error, "reason" ), "Empty field value" );
}

TEST_F( SceneDeviceGroupTests, deviceGroupCheckGroupsEntryDelayInstant ) {
    auto& dbClient = nma().getDBClient();
    const auto AlarmDelay = 0;
    CoreDHouseModes::setAlarmDelay( dbClient, AlarmDelay );
    CoreDRemoteInterface::CoreDInterface& remoteInterface = makeRemote( nma() );
    auto deviceInfo = addDeviceWithSecurityThreatItem( __LINE__ );

    auto deviceId = deviceInfo.m_deviceId;
    std::string devices;
    devices += "[\"" + deviceId + "\"]";

    auto request = replaceTokensInString( scenesDeviceGroupCreateWithDevicesAndDelay,
                                          { { "_DEVICE_GROUP_NAME_", "DeviceGroupWithEntryDelay" },
                                            { "__DEVICES__", devices },
                                            { "__EXCEPTIONS__", "[]" },
                                            { "__ENTRY_DELAY__", "instant" } } );
    sendRequestAndCheckAnswer( Json::parse( request ), __LINE__ );
    JsonObject broadcast = waitForBroadcast( "hub.device.group.created", getCodeLineString( __LINE__ ) );
    auto id = json_get_str( broadcast[ "result" ], "_id" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.group.created" );

    setItemValue( remoteInterface, deviceInfo.m_itemId, true, __LINE__ );

    broadcast = waitForBroadcast( "hub.modes.alarmed", getCodeLineString( __LINE__ ) );
    EXPECT_TRUE( broadcast[ "result" ][ "pendingDelay" ].empty() );
    removeDevice( deviceInfo.m_deviceId, __LINE__ );
    broadcast = waitForBroadcast( "hub.device.group.updated", getCodeLineString( __LINE__ ) );
    EXPECT_FALSE( broadcast[ "result" ].empty() );
    deleteDeviceGroup( id, __LINE__ );
}

TEST_F( SceneDeviceGroupTests, checkGroupAfterDeviceRemoving ) {
    using namespace CoreD::Model::Resources;
    auto deviceInfo = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );
    std::string devices;
    devices += "[\"" + deviceInfo.m_deviceId + "\"]";
    auto exceptionDeviceInfo = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );
    std::string exceptions;
    exceptions += "[\"" + exceptionDeviceInfo.m_deviceId + "\"]";

    auto request = replaceTokensInString( scenesDeviceGroupCreateWithDevices,
                                          { { "_DEVICE_GROUP_NAME_", "devicesAndExceptionsGroup" },
                                            { "__DEVICES__", devices },
                                            { "__EXCEPTIONS__", exceptions } } );
    sendRequestAndCheckAnswer( Json::parse( request ), __LINE__ );
    JsonObject broadcast = waitForBroadcast( "hub.device.group.created", getCodeLineString( __LINE__ ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.group.created" );
    auto id = json_get_str( broadcast[ "result" ], "_id" );
    auto answer = nma().sendtoHubAndGetAnswer( Json::parse( deviceGroupsList ) );
    auto& error = answer[ "error" ];
    EXPECT_TRUE( error.empty() );
    auto& deviceGroups = answer[ "result" ][ "deviceGroups" ];
    removeDevice( exceptionDeviceInfo.m_deviceId, __LINE__ );
    broadcast = waitForBroadcast( "hub.device.group.updated", getCodeLineString( __LINE__ ) );
    auto result = broadcast[ "result" ];
    EXPECT_FALSE( result.empty() );
    auto broadcastName = json_get_str( broadcast[ "result" ], "name" );
    JsonArray broadcastDevicesArray;
    json_get_arr( broadcast[ "result" ], "exceptions", broadcastDevicesArray );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.group.updated" );
    EXPECT_TRUE( broadcastDevicesArray.empty() );
    checkDevicesAndExceptions( id, true, false, __LINE__ );
    removeDevice( deviceInfo.m_deviceId, __LINE__ );
    broadcast = waitForBroadcast( "hub.device.group.updated", getCodeLineString( __LINE__ ) );
    result = broadcast[ "result" ];
    EXPECT_FALSE( result.empty() );
    json_get_arr( broadcast[ "result" ], "exceptions", broadcastDevicesArray );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.group.updated" );
    EXPECT_TRUE( broadcastDevicesArray.empty() );
    checkDevicesAndExceptions( id, false, false, __LINE__ );
    deleteDeviceGroup( id, __LINE__ );
}

TEST_F( SceneDeviceGroupTests, predefineDeviceGroupsListTest ) {  // 9.1
    auto answer = nma().sendtoHubAndGetAnswer( Json::parse( deviceGroupsList ) );
    const auto& error = answer[ "error" ];
    EXPECT_TRUE( error.empty() );
    const auto& deviceGroups = answer[ "result" ][ "deviceGroups" ];
    EXPECT_FALSE( deviceGroups.empty() );
    for( auto group : deviceGroups ) {
        EXPECT_FALSE( group.empty() );
        const auto id = json_get_str( group, "_id" );
        if( id == "fire_sensors" ) {
            EXPECT_EQ( json_get_str( group, "name" ), "Fire Sensor Devices" );
            JsonArray categories;
            json_get_arr( group, "categories", categories );
            EXPECT_EQ( categories.dump(),
                       R"([{"category":"security_sensor","subcategories":["co","gas"]}])" );
            ASSERT_TRUE( json_contains( group, "entryDelay" ) );
            ASSERT_TRUE( json_contains( group, "followEntry" ) );
            ASSERT_TRUE( json_contains( group, "role" ) );
            ASSERT_TRUE( json_contains( group, "packageId" ) );
            EXPECT_EQ( json_get_str( group, "entryDelay" ), "instant" );
            EXPECT_EQ( json_get_str( group, "role" ), "house_modes" );
            EXPECT_EQ( json_get_bool( group, "persistent" ), true );
        }
        else if( id == "life_safety" ) {
            EXPECT_EQ( json_get_str( group, "name" ), "Life Safety Devices" );
            JsonArray categories;
            json_get_arr( group, "categories", categories );
            EXPECT_EQ(
                categories.dump(),
                R"([{"category":"security_sensor","subcategories":["co2","leak","smoke","heat"]},{"category":"scene_controller","subcategories":["panic"]}])" );
            ASSERT_TRUE( json_contains( group, "entryDelay" ) );
            ASSERT_TRUE( json_contains( group, "followEntry" ) );
            ASSERT_TRUE( json_contains( group, "role" ) );
            ASSERT_TRUE( json_contains( group, "packageId" ) );
            EXPECT_EQ( json_get_str( group, "entryDelay" ), "instant" );
            EXPECT_EQ( json_get_str( group, "role" ), "house_modes" );
            EXPECT_EQ( json_get_bool( group, "persistent" ), true );
        }
        else if( id == "sos_button" ) {
            EXPECT_EQ( json_get_str( group, "name" ), "SOS Button Devices" );
            JsonArray categories;
            json_get_arr( group, "categories", categories );
            EXPECT_EQ( categories.dump(),
                       R"([{"category":"sos_button","subcategories":["police_alarm","medical_alarm","fire_alarm"]}])" );
            ASSERT_TRUE( json_contains( group, "entryDelay" ) );
            ASSERT_TRUE( json_contains( group, "followEntry" ) );
            ASSERT_TRUE( json_contains( group, "role" ) );
            ASSERT_TRUE( json_contains( group, "packageId" ) );
            EXPECT_EQ( json_get_str( group, "entryDelay" ), "instant" );
            EXPECT_EQ( json_get_str( group, "role" ), "house_modes" );
            EXPECT_EQ( json_get_bool( group, "persistent" ), true );
        }
    }
}
}  // namespace Tests
}  // namespace HubPlatform
