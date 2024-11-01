#include "bluetooth/entities/device.h"

#include "bluetooth/proxies/gatt_attribute_proxy.h"
#include "bluetooth/proxies/gatt_characteristic_proxy.h"
#include "bluetooth/proxies/gatt_descriptor_proxy.h"
#include "bluetooth/proxies/gatt_service_proxy.h"
#include "bluetooth/ibluetooth_environment.h"
#include "bluetooth/log.h"

#include <algorithm>

namespace HubPlatform {

Device::AttributesTable::AttributesTable( std::string attributesType )
: m_attributesType( std::move( attributesType ) ) {}

void Device::AttributesTable::add( GDBusProxy* proxy ) {
    const GattAttributeProxy attributeProxy( proxy );

    const auto uuid = attributeProxy.getUUID();
    if( m_attributes.find( uuid ) != m_attributes.end() ) {
        LOG_ERROR( "%s '%s' already exists", m_attributesType.c_str(), uuid.c_str() );
    }
    else {
        const auto path = attributeProxy.getProxyPath();
        m_attributes.emplace( uuid, proxy );
        m_pathAssociations.emplace( path, uuid );
        LOG_INFO( "%s '%s' added", m_attributesType.c_str(), uuid.c_str() );
    }
}

void Device::AttributesTable::remove( GDBusProxy* proxy ) {
    const GattAttributeProxy attributeProxy( proxy );

    auto it = m_pathAssociations.find( attributeProxy.getProxyPath() );
    if( it == m_pathAssociations.end() ) {
        LOG_ERROR( "%s '%s' doesn't exist", m_attributesType.c_str(), attributeProxy.getUUID().c_str() );
    }
    else {
        m_attributes.erase( it->second );
        m_pathAssociations.erase( it );
        LOG_INFO( "%s '%s' removed", m_attributesType.c_str(), attributeProxy.getUUID().c_str() );
    }
}

GDBusProxy* Device::AttributesTable::findByUUID( const std::string& uuid ) const {
    auto it = m_attributes.find( uuid );
    return ( it != m_attributes.end() ) ? it->second : nullptr;
}

GDBusProxy* Device::AttributesTable::findByPath( const std::string& path ) const {
    auto it = m_pathAssociations.find( path );
    return ( it != m_pathAssociations.end() ) ? findByUUID( it->second ) : nullptr;
}

Device::AttributesTable::Attributes::const_iterator Device::AttributesTable::begin() const {
    return m_attributes.cbegin();
}

Device::AttributesTable::Attributes::const_iterator Device::AttributesTable::end() const {
    return m_attributes.cend();
}

std::vector< std::string > Device::AttributesTable::getUUIDs() const {
    std::vector< std::string > uuids;
    uuids.reserve( m_attributes.size() );

    std::transform( m_attributes.begin(),
                    m_attributes.end(),
                    std::back_inserter( uuids ),
                    []( const decltype( m_attributes )::value_type& pair ) { return pair.first; } );

    return uuids;
}

Device::Device( IBluetoothEnvironment& bluetoothEnvironment, const DeviceProxy& deviceProxy, const std::string hostMac )
: m_bluetoothEnvironment( bluetoothEnvironment )
, m_hostMac( hostMac )
, m_deviceProxy( deviceProxy )
, m_services( "Service" )
, m_characteristics( "Characteristic" )
, m_descriptors( "Descriptor" ) {}

Device::~Device() = default;

void Device::startNotify( const std::string& uuid ) {
    auto proxy = m_characteristics.findByUUID( uuid );
    if( proxy != nullptr ) {
        GattCharacteristicProxy characteristicProxy( proxy );
        if( characteristicProxy.isNotificationSupported() ) {
            characteristicProxy.startNotify();
        }
        else {
            LOG_ERROR( "Can't start notifications for characteristic '%s', it doesn't support notifications",
                       uuid.c_str() );
        }
    }
    else {
        LOG_ERROR( "Can't start notifications for characteristic '%s', it doesn't exists", uuid.c_str() );
    }
}

void Device::stopNotify( const std::string& uuid ) {
    auto proxy = m_characteristics.findByUUID( uuid );
    if( proxy != nullptr ) {
        GattCharacteristicProxy characteristicProxy( proxy );
        if( characteristicProxy.isNotificationSupported() ) {
            characteristicProxy.stopNotify();
        }
        else {
            LOG_ERROR( "Can't stop notifications for characteristic '%s', it doesn't support notifications",
                       uuid.c_str() );
        }
    }
    else {
        LOG_ERROR( "Can't stop notifications for characteristic '%s', it doesn't exists", uuid.c_str() );
    }
}

void Device::connect() {
    m_deviceProxy.connect();
}

void Device::disconnect() {
    m_deviceProxy.disconnect();
}

void Device::pair() {
    m_deviceProxy.pair();
}

std::string Device::getAddress() const {
    return m_deviceProxy.getAddress();
}

std::string Device::getAdapterPath() const {
    return m_deviceProxy.getAdapterPath();
}

std::string Device::getProxyPath() const {
    return m_deviceProxy.getProxyPath();
}

bool Device::isConnected() const {
    return m_deviceProxy.isConnected();
}

void Device::writeCharacteristic( const std::string& uuid, const std::vector< std::uint8_t >& data ) {
    auto proxy = m_characteristics.findByUUID( uuid );
    if( proxy != nullptr ) {
        GattCharacteristicProxy characteristicProxy( proxy );
        characteristicProxy.writeValue( data );
    }
    else {
        LOG_ERROR( "Can't write characteristic '%s', it doesn't exists", uuid.c_str() );
    }
}

void Device::readCharacteristic( const std::string& uuid ) {
    auto proxy = m_characteristics.findByUUID( uuid );
    if( proxy != nullptr ) {
        GattCharacteristicProxy characteristicProxy( proxy );
        const auto value = characteristicProxy.readValue();
        if( !value.empty() ) {
            m_bluetoothEnvironment.onCharacteristicUpdated( getAddress(), uuid, value );
        }
    }
    else {
        LOG_ERROR( "Can't read characteristic '%s', it doesn't exists", uuid.c_str() );
    }
}

void Device::onPropertyChanged( const std::string& propertyName ) {
    LOG_DEBUG( "Device '%s' property '%s' changed", getAddress().c_str(), propertyName.c_str() );

    if( propertyName == DeviceProxy::ConnectedPropertyName ) {
        const auto address = getAddress();
        if( m_deviceProxy.isConnected() ) {
            m_bluetoothEnvironment.onDeviceConnected( m_hostMac, address, m_characteristics.getUUIDs() );
        }
        else {
            m_bluetoothEnvironment.onDeviceDisconnected( address );
        }
    }
    else if( propertyName == DeviceProxy::PairedPropertyName ) {
        const auto address = getAddress();
        if( m_deviceProxy.isPaired() ) {
            m_bluetoothEnvironment.onDevicePaired( address, m_characteristics.getUUIDs() );
        }
        else {
            m_bluetoothEnvironment.onDeviceUnpaired( address );
        }
    }
}

bool Device::isRelatedAttribute( const GattCharacteristicProxy& attribute ) const {
    return m_services.findByPath( attribute.getServicePath() ) != nullptr;
}

bool Device::isRelatedAttribute( const GattDescriptorProxy& attribute ) const {
    return m_characteristics.findByPath( attribute.getCharacteristicPath() ) != nullptr;
}

bool Device::isRelatedAttribute( const GattServiceProxy& attribute ) const {
    return m_deviceProxy.getProxyPath() == attribute.getDevicePath();
}

void Device::onAttributePropertyChanged( const GattCharacteristicProxy& attribute, const std::string& propertyName ) {
    LOG_INFO(
        "Characteristic '%s' attribute property '%s' changed", attribute.getUUID().c_str(), propertyName.c_str() );

    if( propertyName == GattCharacteristicProxy::ValuePropertyName ) {
        m_bluetoothEnvironment.onCharacteristicUpdated( getAddress(), attribute.getUUID(), attribute.getCachedValue() );
    }
}

void Device::onAttributePropertyChanged( const GattDescriptorProxy& attribute, const std::string& propertyName ) {
    LOG_WARN( "Descriptor property changed is not handled" );
}

void Device::onAttributePropertyChanged( const GattServiceProxy& attribute, const std::string& propertyName ) {
    LOG_WARN( "Service property changed is not handled" );
}

}  // namespace HubPlatform
