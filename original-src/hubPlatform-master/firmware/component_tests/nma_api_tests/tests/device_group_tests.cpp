#include "common_test_resources/resources/uid_resources.h"
#include "cored/model/values/value_factory.h"
#include "common_test_resources/fixtures/core_test_fixture.h"
#include <gtest/gtest.h>

namespace HubPlatform {
namespace Tests {

class DeviceGroupTests : public CoreNmaTest {
public:
    void SetUp() override;
    void TearDown() override;

    std::string createGroup( const std::string& name );
    void updateGroup( const std::string& id,
                      const std::string& name,
                      const std::vector< std::string >& devices,
                      const std::vector< std::string >& exceptions );
    void deleteGroup( const std::string& id );
    JsonObject getGroup( const std::string& id );
    void addDevices( const std::string& id, const std::vector< std::string >& devices );
    void removeDevices( const std::string& id, const std::vector< std::string >& devices );
    void checkChanges( JsonObject recordWas, JsonObject recordNow, JsonObject recordBroadcast );
    void checkDevices( JsonArray was, JsonArray now, JsonArray added, JsonArray removed );

protected:
    std::string m_deviceId1;
    std::string m_deviceId2;
    std::string m_deviceId3;
    std::string m_deviceId4;
    std::string m_deviceId5;

private:
    bool m_persistent = false;
    std::string m_entryDelay = "normal";
    bool m_followEntry = false;
    std::string m_role = "sensor_group";
    std::string m_packageId = "";
    std::vector< std::string > m_devices = {};
    std::vector< std::string > m_exceptions = {};
};

void DeviceGroupTests::SetUp() {
    CoreNmaTest::SetUp();
    callingFile( __FILE__ );
    std::string type = "doorlock";
    std::string deviceTypeId = "297_32770_1536";
    std::string category = "door_lock";
    std::string subcategory = "";
    bool batteryPowered = false;
    m_deviceId1 =
        addDevice( makeRemote(), "Device1", type, deviceTypeId, category, subcategory, batteryPowered, __LINE__ );
    m_deviceId2 =
        addDevice( makeRemote(), "Device2", type, deviceTypeId, category, subcategory, batteryPowered, __LINE__ );
    m_deviceId3 =
        addDevice( makeRemote(), "Device3", type, deviceTypeId, category, subcategory, batteryPowered, __LINE__ );
    m_deviceId4 =
        addDevice( makeRemote(), "Device4", type, deviceTypeId, category, subcategory, batteryPowered, __LINE__ );
    m_deviceId5 =
        addDevice( makeRemote(), "Device5", type, deviceTypeId, category, subcategory, batteryPowered, __LINE__ );
}

void DeviceGroupTests::TearDown() {
    removeDevice( makeRemote(), m_deviceId1, 0, 0, __LINE__ );
    removeDevice( makeRemote(), m_deviceId2, 0, 0, __LINE__ );
    removeDevice( makeRemote(), m_deviceId3, 0, 0, __LINE__ );
    removeDevice( makeRemote(), m_deviceId4, 0, 0, __LINE__ );
    removeDevice( makeRemote(), m_deviceId5, 0, 0, __LINE__ );
    CoreNmaTest::TearDown();
}

std::string DeviceGroupTests::createGroup( const std::string& name ) {
    std::string id;
    JsonObject recordWas = {};
    id = addDeviceGroup( makeRemote(),
                         name,
                         m_persistent,
                         m_entryDelay,
                         m_followEntry,
                         m_role,
                         m_packageId,
                         m_devices,
                         m_exceptions,
                         __LINE__ );
    JsonObject recordNow = getGroup( id );
    JsonObject broadcast = getLastDeviceGroupBroadcast();
    checkChanges( recordWas, recordNow, broadcast );
    return id;
}

void DeviceGroupTests::updateGroup( const std::string& id,
                                    const std::string& name,
                                    const std::vector< std::string >& devices,
                                    const std::vector< std::string >& exceptions ) {
    JsonObject recordWas = getGroup( id );
    updateDeviceGroup(
        makeRemote(), id, name, m_persistent, m_entryDelay, m_followEntry, devices, exceptions, __LINE__ );
    JsonObject recordNow = getGroup( id );
    JsonObject broadcast = getLastDeviceGroupBroadcast();
    checkChanges( recordWas, recordNow, broadcast );
}

void DeviceGroupTests::deleteGroup( const std::string& id ) {
    JsonObject recordWas = getGroup( id );
    removeDeviceGroup( makeRemote(), id, __LINE__ );
    JsonObject recordNow = {};
    JsonObject broadcast = getLastDeviceGroupBroadcast();
    checkChanges( recordWas, recordNow, broadcast );
}

JsonObject DeviceGroupTests::getGroup( const std::string& id ) {
    return getDeviceGroup( makeRemote(), id, __LINE__ );
}

void DeviceGroupTests::addDevices( const std::string& id, const std::vector< std::string >& devices ) {
    JsonObject recordWas = getGroup( id );
    addDeviceGroupDevices( makeRemote(), id, devices, __LINE__ );
    JsonObject recordNow = getGroup( id );
    JsonObject broadcast = getLastDeviceGroupBroadcast();
    checkChanges( recordWas, recordNow, broadcast );
}

void DeviceGroupTests::removeDevices( const std::string& id, const std::vector< std::string >& devices ) {
    JsonObject recordWas = getGroup( id );
    removeDeviceGroupDevices( makeRemote(), id, devices, __LINE__ );
    JsonObject recordNow = getGroup( id );
    JsonObject broadcast = getLastDeviceGroupBroadcast();
    checkChanges( recordWas, recordNow, broadcast );
}

void DeviceGroupTests::checkChanges( JsonObject recordWas, JsonObject recordNow, JsonObject recordBroadcast ) {
    recordBroadcast = recordBroadcast[ "result" ];
    EXPECT_EQ( recordBroadcast[ "role" ], "sensor_group" );
    JsonArray was, now, added, removed;
    was = JsonArray( recordWas[ "devices" ] );
    now = JsonArray( recordNow[ "devices" ] );
    added = JsonArray( recordBroadcast[ "addedDevices" ] );
    removed = JsonArray( recordBroadcast[ "removedDevices" ] );
    checkDevices( was, now, added, removed );
    was = JsonArray( recordWas[ "exceptions" ] );
    now = JsonArray( recordNow[ "exceptions" ] );
    added = JsonArray( recordBroadcast[ "addedExceptions" ] );
    removed = JsonArray( recordBroadcast[ "removedExceptions" ] );
    checkDevices( was, now, added, removed );
}

void DeviceGroupTests::checkDevices( JsonArray was, JsonArray now, JsonArray added, JsonArray removed ) {
    /* tidy up arrays (stop mismatch of null and [])*/
    if( was.empty() ) {
        was = JsonArray();
    }
    if( now.empty() ) {
        now = JsonArray();
    }
    if( added.empty() ) {
        added = JsonArray();
    }
    if( removed.empty() ) {
        removed = JsonArray();
    }
    /* check no ADDED are present in REMOVED */
    for( const auto& device : added ) {
        EXPECT_FALSE( json_array_is_string_present( removed, device ) );
    }
    /* check no REMOVED are present in ADDED */
    for( const auto& device : removed ) {
        EXPECT_FALSE( json_array_is_string_present( added, device ) );
    }
    /* check ADDED are present in NOW and not WAS */
    for( const auto& device : added ) {
        EXPECT_FALSE( json_array_is_string_present( was, device ) );
        EXPECT_TRUE( json_array_is_string_present( now, device ) );
    }
    /* check REMOVED are present in WAS and not NOW */
    for( const auto& device : removed ) {
        EXPECT_TRUE( json_array_is_string_present( was, device ) );
        EXPECT_FALSE( json_array_is_string_present( now, device ) );
    }
    /* everything in WAS... */
    for( const auto& device : was ) {
        /* ...and is in NOW must not be in REMOVED or ADDED */
        if( json_array_is_string_present( now, device ) ) {
            EXPECT_FALSE( json_array_is_string_present( removed, device ) );
            EXPECT_FALSE( json_array_is_string_present( added, device ) );
        }
        else {
            /* ...else if not in NOW must be in REMOVED and not in ADDED */
            EXPECT_TRUE( json_array_is_string_present( removed, device ) );
            EXPECT_FALSE( json_array_is_string_present( added, device ) );
        }
    }
    /* everything in NOW... */
    for( const auto& device : now ) {
        /* ...and is in WAS must not be in REMOVED or ADDED */
        if( json_array_is_string_present( was, device ) ) {
            EXPECT_FALSE( json_array_is_string_present( removed, device ) );
            EXPECT_FALSE( json_array_is_string_present( added, device ) );
        }
        else {
            /* ...else if not in WAS must be in ADDED and not in REMOVED */
            EXPECT_TRUE( json_array_is_string_present( added, device ) );
            EXPECT_FALSE( json_array_is_string_present( removed, device ) );
        }
    }
}

TEST_F( DeviceGroupTests, createAndDestroyGroup ) {
    std::string name = "TestGroup";
    std::string deviceGroupId;
    deviceGroupId = createGroup( name );
    deleteGroup( deviceGroupId );
}

TEST_F( DeviceGroupTests, updateGroupDevices ) {
    std::string name = "TestGroupForDevices";
    std::string deviceGroupId;
    deviceGroupId = createGroup( name );
    updateGroup( deviceGroupId, name, { m_deviceId1, m_deviceId2, m_deviceId3, m_deviceId4, m_deviceId5 }, {} );
    updateGroup( deviceGroupId, name, { m_deviceId1, m_deviceId5 }, {} );
    updateGroup( deviceGroupId, name, { m_deviceId2, m_deviceId3, m_deviceId4 }, {} );
    updateGroup( deviceGroupId, name, { m_deviceId1, m_deviceId2, m_deviceId4, m_deviceId5 }, {} );
    deleteGroup( deviceGroupId );
}

TEST_F( DeviceGroupTests, updateGroupExceptions ) {
    std::string name = "TestGroupForExceptions";
    std::string deviceGroupId;
    deviceGroupId = createGroup( name );
    updateGroup( deviceGroupId, name, {}, { m_deviceId1, m_deviceId2, m_deviceId3, m_deviceId4, m_deviceId5 } );
    updateGroup( deviceGroupId, name, {}, { m_deviceId1, m_deviceId5 } );
    updateGroup( deviceGroupId, name, {}, { m_deviceId2, m_deviceId3, m_deviceId4 } );
    updateGroup( deviceGroupId, name, {}, { m_deviceId1, m_deviceId2, m_deviceId4, m_deviceId5 } );
    deleteGroup( deviceGroupId );
}

TEST_F( DeviceGroupTests, updateGroupDevicesAndExceptions ) {
    std::string name = "TestGroupForDevicesAndExceptions";
    std::string deviceGroupId;
    deviceGroupId = createGroup( name );
    updateGroup( deviceGroupId,
                 name,
                 { m_deviceId1, m_deviceId2, m_deviceId3, m_deviceId4, m_deviceId5 },
                 { m_deviceId1, m_deviceId2, m_deviceId3, m_deviceId4, m_deviceId5 } );
    updateGroup( deviceGroupId, name, { m_deviceId1, m_deviceId5 }, { m_deviceId1, m_deviceId5 } );
    updateGroup(
        deviceGroupId, name, { m_deviceId2, m_deviceId3, m_deviceId4 }, { m_deviceId2, m_deviceId3, m_deviceId4 } );
    updateGroup( deviceGroupId,
                 name,
                 { m_deviceId1, m_deviceId2, m_deviceId4, m_deviceId5 },
                 { m_deviceId1, m_deviceId2, m_deviceId4, m_deviceId5 } );
    deleteGroup( deviceGroupId );
}

TEST_F( DeviceGroupTests, updateGroupDevicesViaDeviceAddRemove ) {
    std::string name = "TestGroupDevicesViaDeviceAdd";
    std::string deviceGroupId;
    deviceGroupId = createGroup( name );
    /* add 5 devices */
    addDevices( deviceGroupId, { m_deviceId1, m_deviceId2, m_deviceId3, m_deviceId4, m_deviceId5 } );
    /* remove 4 and 5 */
    removeDevices( deviceGroupId, { m_deviceId4, m_deviceId5 } );
    /* add 3, 4 and 5; where 3 is already present so ignored */
    addDevices( deviceGroupId, { m_deviceId3, m_deviceId4, m_deviceId5 } );
    /* remove all 5; where some are already removed, so ignored */
    removeDevices( deviceGroupId, { m_deviceId1, m_deviceId2, m_deviceId3, m_deviceId4, m_deviceId5 } );
    /* add 1, several times */
    addDevices( deviceGroupId, { m_deviceId2, m_deviceId2, m_deviceId2 } );
    /* add 5 devices */
    addDevices( deviceGroupId, { m_deviceId1, m_deviceId2, m_deviceId3, m_deviceId4, m_deviceId5 } );
    /* remove the 1st, last and middle */
    removeDevices( deviceGroupId, { m_deviceId1, m_deviceId5, m_deviceId3 } );
    deleteGroup( deviceGroupId );
}

}  // namespace Tests
}  // namespace HubPlatform
