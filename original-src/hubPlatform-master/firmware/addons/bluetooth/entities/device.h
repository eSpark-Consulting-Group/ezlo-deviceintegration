#pragma once

#include "bluetooth/proxies/device_proxy.h"

#include <unordered_map>

namespace HubPlatform {

class GattCharacteristicProxy;
class GattDescriptorProxy;
class GattServiceProxy;
class IBluetoothEnvironment;

class Device {
public:
    Device( IBluetoothEnvironment& bluetoothEnvironment, const DeviceProxy& deviceProxy, const std::string hostMac );
    ~Device();

    void startNotify( const std::string& uuid );
    void stopNotify( const std::string& uuid );
    void disconnect();
    void connect();
    void pair();

    std::string getAddress() const;
    std::string getAdapterPath() const;
    std::string getProxyPath() const;
    bool isConnected() const;

    template < typename T >
    void addAttribute( const T& attribute );

    template < typename T >
    void removeAttribute( const T& attribute );

    void writeCharacteristic( const std::string& uuid, const std::vector< std::uint8_t >& data );
    void readCharacteristic( const std::string& uuid );

    void onPropertyChanged( const std::string& propertyName );

    void onAttributePropertyChanged( const GattCharacteristicProxy& attribute, const std::string& propertyName );
    void onAttributePropertyChanged( const GattDescriptorProxy& attribute, const std::string& propertyName );
    void onAttributePropertyChanged( const GattServiceProxy& attribute, const std::string& propertyName );

    bool isRelatedAttribute( const GattCharacteristicProxy& attribute ) const;
    bool isRelatedAttribute( const GattDescriptorProxy& attribute ) const;
    bool isRelatedAttribute( const GattServiceProxy& attribute ) const;

private:
    class AttributesTable final {
    private:
        using Attributes = std::unordered_map< std::string, GDBusProxy* >;
        using PathAssociations = std::unordered_map< std::string, std::string >;

    public:
        explicit AttributesTable( std::string attributesType );

        void add( GDBusProxy* proxy );
        void remove( GDBusProxy* proxy );

        GDBusProxy* findByUUID( const std::string& uuid ) const;
        GDBusProxy* findByPath( const std::string& path ) const;

        Attributes::const_iterator begin() const;
        Attributes::const_iterator end() const;

        std::vector< std::string > getUUIDs() const;

    private:
        const std::string m_attributesType;

        PathAssociations m_pathAssociations;
        Attributes m_attributes;
    };

    template < typename T >
    AttributesTable& getAttributesTable();

private:
    IBluetoothEnvironment& m_bluetoothEnvironment;
    std::string m_hostMac;

    DeviceProxy m_deviceProxy;

    AttributesTable m_services;
    AttributesTable m_characteristics;
    AttributesTable m_descriptors;
};

template < typename T >
inline void Device::addAttribute( const T& attribute ) {
    getAttributesTable< T >().add( attribute.getProxyPtr() );
}

template < typename T >
inline void Device::removeAttribute( const T& attribute ) {
    getAttributesTable< T >().remove( attribute.getProxyPtr() );
}

template <>
inline Device::AttributesTable& Device::getAttributesTable< GattCharacteristicProxy >() {
    return m_characteristics;
}

template <>
inline Device::AttributesTable& Device::getAttributesTable< GattServiceProxy >() {
    return m_services;
}

template <>
inline Device::AttributesTable& Device::getAttributesTable< GattDescriptorProxy >() {
    return m_descriptors;
}

}  // namespace HubPlatform
