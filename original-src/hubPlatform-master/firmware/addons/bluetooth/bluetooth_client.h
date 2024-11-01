#pragma once

#include "service_interfaces/script_client.h"
#include "addon_interfaces/addon.h"

#include <chrono>

namespace HubPlatform {

class BluetoothClient : public ScriptClient {
public:
    BluetoothClient( Addons::Addon& hostAddon );

    Json getDeviceAsJson( const std::string& deviceAddress );
    void maintainConnection( const std::string& deviceAddress );
    void startNotify( const std::string& deviceAddress, const std::string& uuid );
    void stopNotify( const std::string& deviceAddress, const std::string& uuid );
    void startDiscovery();
    void stopDiscovery();
    void removeDevice( const std::string& deviceAddress );
    void connectToDevice( const std::string& deviceAddress );
    void disconnectDevice( const std::string& deviceAddress );
    void pair( const std::string& deviceAddress );
    void writeCharacteristic( const std::string& deviceAddress,
                              const std::string& uuid,
                              const std::vector< std::uint8_t >& data );
    void readCharacteristic( const std::string& deviceAddress, const std::string& uuid );
};

}  // namespace HubPlatform
