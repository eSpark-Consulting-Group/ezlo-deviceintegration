#include "bluetooth/entities/adapter.h"

#include "advertising_manager.h"
#include "bluetooth/entities/device.h"
#include "bluetooth/proxies/advertising_manager_proxy.h"
#include "bluetooth/ibluetooth_environment.h"
#include "bluetooth/log.h"

#include "common_utils/cpp14.h"

namespace HubPlatform {

Adapter::Adapter( DBusConnection* dbusConnection,
                  IBluetoothEnvironment& bluetoothEnvironment,
                  const AdapterProxy& adapterProxy )
: m_dbusConnection( dbusConnection )
, m_bluetoothEnvironment( bluetoothEnvironment )
, m_adapterProxy( adapterProxy ) {}

std::string Adapter::getAddress() {
    return m_adapterProxy.getAddress();
}

void Adapter::setupAdvertisingManagerProxy( const AdvertisingManagerProxy& advertisingManagerProxy ) {
    m_advertisingManager = std::make_unique< AdvertisingManager >( m_dbusConnection, advertisingManagerProxy );
}

void Adapter::startAdvertising( const std::string& localName,
                                const std::vector< std::string >& advertiseServiceUUIDs ) {
    if( m_advertisingManager ) {
        m_advertisingManager->startAdvertising( localName, advertiseServiceUUIDs );
    }
}

void Adapter::stopAdvertising() {
    if( m_advertisingManager ) {
        m_advertisingManager->stopAdvertising();
    }
}

void Adapter::registerGattApplication( const std::string& serviceUUID, const std::string& characterUUID ) {
    if( m_gattManager ) {
        m_gattManager->registerApplication( serviceUUID, characterUUID );
    }
}

void Adapter::unregisterGattApplication() {
    /*TODO*/
}

void Adapter::setPowered( bool powered ) {
    m_adapterProxy.setPowered( powered );
}

void Adapter::setDiscoverable( bool discoverable ) {
    m_adapterProxy.setDiscoverable( discoverable );
}

void Adapter::startDiscovery() {
    m_adapterProxy.startDiscovery();
}

void Adapter::stopDiscovery() {
    m_adapterProxy.stopDiscovery();
}

void Adapter::connectToDevice( const std::string& deviceAddress ) {
    auto it = m_devices.find( deviceAddress );
    if( it != m_devices.end() ) {
        auto& device = it->second;
        device.connect();
    }
    else {
        LOG_ERROR( "Can't connect to device '%s'", deviceAddress.c_str() );
    }
}

void Adapter::disconnectDevice( const std::string& deviceAddress ) {
    auto it = m_devices.find( deviceAddress );
    if( it != m_devices.end() ) {
        auto& device = it->second;
        device.disconnect();
    }
    else {
        LOG_ERROR( "Can't disconnect device '%s'", deviceAddress.c_str() );
    }
}

void Adapter::pair( const std::string& deviceAddress ) {
    auto it = m_devices.find( deviceAddress );
    if( it != m_devices.end() ) {
        auto& device = it->second;
        device.pair();
    }
    else {
        LOG_ERROR( "Can't pair device '%s'", deviceAddress.c_str() );
    }
}

void Adapter::addDevice( const DeviceProxy& deviceProxy ) {
    const auto address = deviceProxy.getAddress();
    if( address.empty() ) {
        LOG_ERROR( "Can't add device with invalid address" );
    }
    else if( m_devices.find( address ) != m_devices.end() ) {
        LOG_ERROR( "Device '%s' already exists", address.c_str() );
    }
    else {
        m_devices.emplace( address, Device( m_bluetoothEnvironment, deviceProxy, getAddress() ) );
        m_bluetoothEnvironment.onDeviceDiscovered( address );
        LOG_INFO( "Device '%s' added", address.c_str() );
    }
}

void Adapter::removeDevice( const std::string& deviceAddress ) {
    auto it = m_devices.find( deviceAddress );
    if( it != m_devices.end() ) {
        auto& device = it->second;
        m_adapterProxy.removeDevice( device.getProxyPath() );
    }
    else {
        LOG_ERROR( "Can't remove device '%s'", deviceAddress.c_str() );
    }
}

void Adapter::removeDevice( const DeviceProxy& deviceProxy ) {
    const auto address = deviceProxy.getAddress();

    auto it = m_devices.find( address );
    if( it == m_devices.end() ) {
        LOG_ERROR( "Device '%s' was not added", address.c_str() );
    }
    else {
        m_devices.erase( it );

        auto addressIt = std::find( m_autoConnectDevices.begin(), m_autoConnectDevices.end(), address );
        if( addressIt != m_autoConnectDevices.end() ) {
            m_autoConnectDevices.erase( addressIt );
        }

        m_bluetoothEnvironment.onDeviceRemoved( address );
        LOG_INFO( "Device '%s' removed", address.c_str() );
    }
}

void Adapter::writeCharacteristic( const std::string& deviceAddress,
                                   const std::string& uuid,
                                   const std::vector< std::uint8_t >& data ) {
    auto it = m_devices.find( deviceAddress );
    if( it != m_devices.end() ) {
        auto& device = it->second;
        device.writeCharacteristic( uuid, data );
    }
    else {
        LOG_ERROR( "Can't write device '%s' characteristic. Device doesn't exist", deviceAddress.c_str() );
    }
}

void Adapter::readCharacteristic( const std::string& deviceAddress, const std::string& uuid ) {
    auto it = m_devices.find( deviceAddress );
    if( it != m_devices.end() ) {
        auto& device = it->second;
        device.readCharacteristic( uuid );
    }
    else {
        LOG_ERROR( "Can't read device '%s' characteristic. Device doesn't exist", deviceAddress.c_str() );
    }
}

void Adapter::setupGattManager( const GattManagerProxy& gattManagerProxy ) {
    m_gattManager = std::make_unique< GattManager >( m_dbusConnection, gattManagerProxy );
}

std::string Adapter::getProxyPath() {
    return m_adapterProxy.getProxyPath();
}

void Adapter::onPropertyChanged( const std::string& propertyName ) {
    if( propertyName == AdapterProxy::DiscoveringPropertyName ) {
        if( m_adapterProxy.isDiscovering() ) {
            m_bluetoothEnvironment.onDiscoveryStarted();
        }
        else {
            m_bluetoothEnvironment.onDiscoveryStopped();
        }
    }
}

void Adapter::onDevicePropertyChanged( const DeviceProxy& deviceProxy, const std::string& propertyName ) {
    const auto address = deviceProxy.getAddress();
    auto it = m_devices.find( address );
    if( it != m_devices.end() ) {
        auto& device = it->second;
        device.onPropertyChanged( propertyName );
    }
    else {
        LOG_ERROR( "Can't handle property change of device '%s'", address.c_str() );
    }
}

void Adapter::startNotify( const std::string& deviceAddress, const std::string& uuid ) {
    auto it = m_devices.find( deviceAddress );
    if( it != m_devices.end() ) {
        auto& device = it->second;
        device.startNotify( uuid );
    }
    else {
        LOG_ERROR( "Can't start notifications for device '%s'", deviceAddress.c_str() );
    }
}

void Adapter::stopNotify( const std::string& deviceAddress, const std::string& uuid ) {
    auto it = m_devices.find( deviceAddress );
    if( it != m_devices.end() ) {
        auto& device = it->second;
        device.stopNotify( uuid );
    }
    else {
        LOG_ERROR( "Can't stop notifications for device '%s'", deviceAddress.c_str() );
    }
}

void Adapter::updateConnections() {
    for( const auto& deviceAddress : m_autoConnectDevices ) {
        auto it = m_devices.find( deviceAddress );
        if( it != m_devices.end() ) {
            auto& device = it->second;
            if( !device.isConnected() ) {
                device.connect();
            }
        }
        else {
            LOG_ERROR( "Can't auto connect to device '%s'", deviceAddress.c_str() );
        }
    }
}

void Adapter::maintainConnection( const std::string& deviceAddress ) {
    if( m_devices.count( deviceAddress ) > 0 ) {
        m_autoConnectDevices.push_back( deviceAddress );
        LOG_ERROR( "Connection maintaining started for device '%s'", deviceAddress.c_str() );
    }
    else {
        LOG_ERROR( "Can't maintain connection with device '%s'", deviceAddress.c_str() );
    }
}

}  // namespace HubPlatform
