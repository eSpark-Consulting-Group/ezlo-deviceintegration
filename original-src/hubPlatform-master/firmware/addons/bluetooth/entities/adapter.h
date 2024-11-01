#pragma once

#include "bluetooth/proxies/adapter_proxy.h"
#include "bluetooth/proxies/gatt_service_proxy.h"
#include "bluetooth/entities/advertising_manager.h"
#include "bluetooth/entities/device.h"
#include "bluetooth/entities/gatt_manager.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace HubPlatform {

class AdvertisingManager;
class Device;
class DeviceProxy;
class AdvertisingManagerProxy;
class GattManagerProxy;
class IBluetoothEnvironment;

class Adapter {
public:
    Adapter( DBusConnection* dbusConnection,
             IBluetoothEnvironment& bluetoothEnvironment,
             const AdapterProxy& adapterProxy );

    void setupAdvertisingManagerProxy( const AdvertisingManagerProxy& advertisingManagerProxy );
    void startAdvertising( const std::string& localName, const std::vector< std::string >& advertiseServiceUUIDs );
    void stopAdvertising();

    void registerGattApplication( const std::string& serviceUUID, const std::string& characterUUID );
    void unregisterGattApplication();

    void setPowered( bool powered = true );
    void setDiscoverable( bool discoverable = true );

    void startDiscovery();
    void stopDiscovery();

    void startNotify( const std::string& deviceAddress, const std::string& uuid );
    void stopNotify( const std::string& deviceAddress, const std::string& uuid );

    void connectToDevice( const std::string& deviceAddress );
    void disconnectDevice( const std::string& deviceAddress );
    void pair( const std::string& deviceAddress );
    void addDevice( const DeviceProxy& deviceProxy );

    template < typename T >
    void addDeviceAttribute( const T& attribute );
    template < typename T >
    void removeDeviceAttribute( const T& attribute );

    void setupGattManager( const GattManagerProxy& gattManagerProxy );

    std::string getProxyPath();

    void removeDevice( const std::string& deviceAddress );
    void removeDevice( const DeviceProxy& deviceProxy );

    void writeCharacteristic( const std::string& deviceAddress,
                              const std::string& uuid,
                              const std::vector< std::uint8_t >& data );
    void readCharacteristic( const std::string& deviceAddress, const std::string& uuid );

    std::string getAddress();

    void onPropertyChanged( const std::string& propertyName );
    void onDevicePropertyChanged( const DeviceProxy& deviceProxy, const std::string& propertyName );

    template < typename T >
    void onAttributePropertyChanged( const T& attribute, const std::string& propertyName );

    void maintainConnection( const std::string& deviceAddress );
    void updateConnections();

private:
    DBusConnection* m_dbusConnection;
    AdapterProxy m_adapterProxy;
    std::unique_ptr< AdvertisingManager > m_advertisingManager;
    std::unique_ptr< GattManager > m_gattManager;
    std::unordered_map< std::string, Device > m_devices;
    std::vector< std::string > m_autoConnectDevices;
    IBluetoothEnvironment& m_bluetoothEnvironment;
};

template < typename T >
inline void Adapter::addDeviceAttribute( const T& attribute ) {
    for( auto& pair : m_devices ) {
        auto& device = pair.second;
        if( device.isRelatedAttribute( attribute ) ) {
            device.addAttribute( attribute );
            break;
        }
    }
}

template < typename T >
inline void Adapter::removeDeviceAttribute( const T& attribute ) {
    for( auto& pair : m_devices ) {
        auto& device = pair.second;
        if( device.isRelatedAttribute( attribute ) ) {
            device.removeAttribute( attribute );
            break;
        }
    }
}

template < typename T >
inline void Adapter::onAttributePropertyChanged( const T& attribute, const std::string& propertyName ) {
    for( auto& pair : m_devices ) {
        auto& device = pair.second;
        if( device.isRelatedAttribute( attribute ) ) {
            device.onAttributePropertyChanged( attribute, propertyName );
            break;
        }
    }
}

}  // namespace HubPlatform
