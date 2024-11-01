#pragma once

#include <string>
#include <vector>

namespace HubPlatform {

class IBluetoothManager {
public:
    virtual ~IBluetoothManager() = default;

    virtual void init() = 0;

    virtual void startServiceAP( const std::string& localName,
                                 const std::string& pinCode,
                                 const std::string& serviceUUID,
                                 const std::string& characterUUID ) = 0;
    virtual void stopServiceAP() = 0;

    virtual void startDiscovery() = 0;
    virtual void stopDiscovery() = 0;

    virtual void maintainConnection( const std::string& deviceAddress ) = 0;

    virtual void startNotify( const std::string& deviceAddress, const std::string& uuid ) = 0;
    virtual void stopNotify( const std::string& deviceAddress, const std::string& uuid ) = 0;

    virtual void removeDevice( const std::string& deviceAddress ) = 0;
    virtual void connectToDevice( const std::string& deviceAddress ) = 0;
    virtual void disconnectDevice( const std::string& deviceAddress ) = 0;
    virtual void pair( const std::string& deviceAddress ) = 0;

    virtual void writeCharacteristic( const std::string& deviceAddress,
                                      const std::string& uuid,
                                      const std::vector< std::uint8_t >& data ) = 0;
    virtual void readCharacteristic( const std::string& deviceAddress, const std::string& uuid ) = 0;
};

}  // namespace HubPlatform
