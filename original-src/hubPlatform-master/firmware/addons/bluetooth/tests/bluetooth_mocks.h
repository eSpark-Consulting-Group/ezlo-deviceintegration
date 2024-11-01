#pragma once

#include "bluetooth/ibluetooth_environment.h"
#include "bluetooth/ibluetooth_manager.h"
#include "bluetooth/bluetooth_addon.h"

#include "addon_interfaces/addon.h"
#include "service_interfaces/bus_wrapper.h"
#include "bus/base_bus.h"
#include "bus/fake_bus.h"
#include "database_client/fake_db/fake_db_client.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace HubPlatform {
namespace UT {

class MockBluetoothEnvironment : public IBluetoothEnvironment {
public:
    MockBluetoothEnvironment( const BluetoothAddon& ) {}

    MOCK_METHOD0( onDiscoveryStopped, void() );
    MOCK_METHOD0( onDiscoveryStarted, void() );

    MOCK_METHOD1( onDeviceDiscovered, void( const std::string& ) );
    MOCK_METHOD1( onDeviceRemoved, void( const std::string& ) );
    MOCK_METHOD3( onDeviceConnected,
                  void( const std::string&, const std::string&, const std::vector< std::string >& ) );
    MOCK_METHOD1( onDeviceDisconnected, void( const std::string& ) );
    MOCK_METHOD2( onDevicePaired, void( const std::string&, const std::vector< std::string >& ) );
    MOCK_METHOD1( onDeviceUnpaired, void( const std::string& ) );
    MOCK_METHOD3( onCharacteristicUpdated,
                  void( const std::string&, const std::string&, const std::vector< std::uint8_t >& ) );
};

class MockBluetoothManager : public IBluetoothManager {
public:
    MockBluetoothManager( const IBluetoothEnvironment& ) {}

    void init() override {}

    MOCK_METHOD4( startServiceAP,
                  void( const std::string&, const std::string&, const std::string&, const std::string& ) );
    MOCK_METHOD0( stopServiceAP, void() );

    MOCK_METHOD0( startDiscovery, void() );
    MOCK_METHOD0( stopDiscovery, void() );

    MOCK_METHOD1( maintainConnection, void( const std::string& ) );

    MOCK_METHOD2( startNotify, void( const std::string&, const std::string& ) );
    MOCK_METHOD2( stopNotify, void( const std::string&, const std::string& ) );

    MOCK_METHOD1( removeDevice, void( const std::string& ) );
    MOCK_METHOD1( connectToDevice, void( const std::string& ) );
    MOCK_METHOD1( disconnectDevice, void( const std::string& ) );
    MOCK_METHOD1( pair, void( const std::string& ) );
    MOCK_METHOD3( writeCharacteristic,
                  void( const std::string&, const std::string&, const std::vector< std::uint8_t >& ) );
    MOCK_METHOD2( readCharacteristic, void( const std::string&, const std::string& ) );
};

}  // namespace UT
}  // namespace HubPlatform
