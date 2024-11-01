#pragma once

#include "bluetooth/ibluetooth_environment.h"

namespace HubPlatform {

namespace Addons {
class Addon;
}

class BluetoothEnvironment : public IBluetoothEnvironment {
public:
    BluetoothEnvironment( Addons::Addon& addon );

    void onDiscoveryStopped() override;
    void onDiscoveryStarted() override;

    void onDeviceDiscovered( const std::string& deviceAddress ) override;
    void onDeviceRemoved( const std::string& deviceAddress ) override;
    void onDeviceConnected( const std::string& hostAddress,
                            const std::string& deviceAddress,
                            const std::vector< std::string >& characteristicUUIDs ) override;
    void onDeviceDisconnected( const std::string& deviceAddress ) override;
    void onDevicePaired( const std::string& deviceAddress,
                         const std::vector< std::string >& characteristicUUIDs ) override;
    void onDeviceUnpaired( const std::string& deviceAddress ) override;
    void onCharacteristicUpdated( const std::string& deviceAddress,
                                  const std::string& characteristicUUID,
                                  const std::vector< std::uint8_t >& value ) override;

private:
    void removeDeviceFromDB( const std::string& deviceAddress );
    void addDeviceToDB( const std::string& deviceAddress, const std::vector< std::string >& characteristicUUIDs );

private:
    Addons::Addon& m_addon;
};

}  // namespace HubPlatform
