#pragma once

#include <string>
#include <vector>

namespace HubPlatform {

class IBluetoothEnvironment {
public:
    virtual ~IBluetoothEnvironment() = default;

    virtual void onDiscoveryStopped() = 0;
    virtual void onDiscoveryStarted() = 0;

    virtual void onDeviceDiscovered( const std::string& deviceAddress ) = 0;
    virtual void onDeviceRemoved( const std::string& deviceAddress ) = 0;
    virtual void onDeviceConnected( const std::string& hostAddress,
                                    const std::string& deviceAddress,
                                    const std::vector< std::string >& characteristicUUIDs ) = 0;
    virtual void onDeviceDisconnected( const std::string& deviceAddress ) = 0;
    virtual void onDevicePaired( const std::string& deviceAddress,
                                 const std::vector< std::string >& characteristicUUIDs ) = 0;
    virtual void onDeviceUnpaired( const std::string& deviceAddress ) = 0;
    virtual void onCharacteristicUpdated( const std::string& deviceAddress,
                                          const std::string& characteristicUUID,
                                          const std::vector< std::uint8_t >& value ) = 0;
};

}  // namespace HubPlatform
