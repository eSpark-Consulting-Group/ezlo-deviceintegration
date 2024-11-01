#include "bluetooth/script_events/characteristic_updated.h"
#include "bluetooth/script_events/device_connected.h"
#include "bluetooth/script_events/device_disconnected.h"
#include "bluetooth/script_events/device_discovered.h"
#include "bluetooth/script_events/device_removed.h"
#include "bluetooth/script_events/device_paired.h"
#include "bluetooth/script_events/device_unpaired.h"
#include "bluetooth/script_events/discovery_started.h"
#include "bluetooth/script_events/discovery_stopped.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace HubPlatform {
namespace UT {

TEST( BluetoothEventToJSON, characteristicUpdatedEvent ) {
    CharacteristicUpdated event( "test_address", "test_uuid", { 0x1, 0x2, 0x3 } );
    const auto joEvent = event.toJson();
    ASSERT_EQ( joEvent.size(), 4 );
    EXPECT_EQ( joEvent[ "event" ], "characteristic_updated" );
    EXPECT_EQ( joEvent[ "deviceAddress" ], "test_address" );
    EXPECT_EQ( joEvent[ "characteristicUUID" ], "test_uuid" );
    EXPECT_EQ( joEvent[ "value" ], std::vector< uint8_t >( { 0x1, 0x2, 0x3 } ) );
}

TEST( BluetoothEventToJSON, deviceConnectedEvent ) {
    DeviceConnected event( "test_address" );
    const auto joEvent = event.toJson();
    ASSERT_EQ( joEvent.size(), 2 );
    EXPECT_EQ( joEvent[ "event" ], "device_connected" );
    EXPECT_EQ( joEvent[ "deviceAddress" ], "test_address" );
}

TEST( BluetoothEventToJSON, deviceDisconnectedEvent ) {
    DeviceDisconnected event( "test_address" );
    const auto joEvent = event.toJson();
    ASSERT_EQ( joEvent.size(), 2 );
    EXPECT_EQ( joEvent[ "event" ], "device_disconnected" );
    EXPECT_EQ( joEvent[ "deviceAddress" ], "test_address" );
}

TEST( BluetoothEventToJSON, deviceDiscoveredEvent ) {
    DeviceDiscovered event( "test_address" );
    const auto joEvent = event.toJson();
    ASSERT_EQ( joEvent.size(), 2 );
    EXPECT_EQ( joEvent[ "event" ], "device_discovered" );
    EXPECT_EQ( joEvent[ "deviceAddress" ], "test_address" );
}

TEST( BluetoothEventToJSON, deviceRemovedEvent ) {
    DeviceRemoved event( "test_address" );
    const auto joEvent = event.toJson();
    ASSERT_EQ( joEvent.size(), 2 );
    EXPECT_EQ( joEvent[ "event" ], "device_removed" );
    EXPECT_EQ( joEvent[ "deviceAddress" ], "test_address" );
}

TEST( BluetoothEventToJSON, devicePairedEvent ) {
    DevicePaired event( "test_address" );
    const auto joEvent = event.toJson();
    ASSERT_EQ( joEvent.size(), 2 );
    EXPECT_EQ( joEvent[ "event" ], "device_paired" );
    EXPECT_EQ( joEvent[ "deviceAddress" ], "test_address" );
}

TEST( BluetoothEventToJSON, deviceUnpairedEvent ) {
    DeviceUnpaired event( "test_address" );
    const auto joEvent = event.toJson();
    ASSERT_EQ( joEvent.size(), 2 );
    EXPECT_EQ( joEvent[ "event" ], "device_unpaired" );
    EXPECT_EQ( joEvent[ "deviceAddress" ], "test_address" );
}

TEST( BluetoothEventToJSON, discoveryStartedEvent ) {
    DiscoveryStarted event;
    const auto joEvent = event.toJson();
    ASSERT_EQ( joEvent.size(), 1 );
    EXPECT_EQ( joEvent[ "event" ], "discovery_started" );
}

TEST( BluetoothEventToJSON, discoveryStoppedEvent ) {
    DiscoveryStopped event;
    const auto joEvent = event.toJson();
    ASSERT_EQ( joEvent.size(), 1 );
    EXPECT_EQ( joEvent[ "event" ], "discovery_stopped" );
}
}  // namespace UT
}  // namespace HubPlatform
