#include "device_descriptions_container.h"
#include <type_traits>

DeviceDescriptionsContainer::DeviceDescriptionsContainer() = default;

DeviceDescriptionsContainer::DeviceDescriptionsContainer( ManufacturersNames&& names,
                                                          DeviceDescriptions&& descriptions,
                                                          DeviceTypesList&& types,
                                                          ItemSubTypesList&& subTypes,
                                                          DeviceSubTypesList&& deviceSubTypes ) {
    setDeviceManufacturers( std::move( names ) );
    setDeviceDescriptions( std::move( descriptions ) );
    setDeviceTypes( std::move( types ) );
    setDeviceItemSubtypes( std::move( subTypes ) );
    setDeviceSubtypes( std::move( deviceSubTypes ) );
}

void DeviceDescriptionsContainer::setDeviceManufacturers( ManufacturersNames&& names ) {
    m_manufacturersNames = std::move( names );
}

void DeviceDescriptionsContainer::setDevicesShowAsRuleExcludedItems( DevicesShowAsRuleExcludedItems&& items ) {
    m_devicesShowAsRuleExcludedItems = std::move( items );
}

void DeviceDescriptionsContainer::setDeviceDescriptions( DeviceDescriptions&& descriptions ) {
    m_deviceDescriptions = std::move( descriptions );
}

void DeviceDescriptionsContainer::setDeviceTypes( DeviceTypesList&& typesList ) {
    m_deviceTypes = std::move( typesList );
}

void DeviceDescriptionsContainer::setDeviceItemSubtypes( ItemSubTypesList&& itemSubTypesList ) {
    m_deviceItemSubTypes = std::move( itemSubTypesList );
}

void DeviceDescriptionsContainer::setDeviceSubtypes( DeviceSubTypesList&& subTypesList ) {
    m_deviceSubTypes = std::move( subTypesList );
}

const DeviceInfoPtr& DeviceDescriptionsContainer::findDeviceInfo( const DeviceId& id ) const {
    return findOrGetDefaultValue( m_deviceDescriptions, id );
}

const std::vector< std::string >& DeviceDescriptionsContainer::findDeviceShowAsRuleExcludedItem(
    const std::string& id ) const {
    return findOrGetDefaultValue( m_devicesShowAsRuleExcludedItems, id );
}

const std::string& DeviceDescriptionsContainer::findDeviceManufacturer( int id ) const {
    return findOrGetDefaultValue( m_manufacturersNames, id );
}

const std::string& DeviceDescriptionsContainer::findDeviceType( int id ) const {
    return findOrGetDefaultValue( m_deviceTypes, id );
}

const std::string& DeviceDescriptionsContainer::findDeviceSubType( int id ) const {
    return findOrGetDefaultValue( m_deviceSubTypes, id );
}

const std::string& DeviceDescriptionsContainer::findDeviceItemSubType( int id ) const {
    return findOrGetDefaultValue( m_deviceItemSubTypes, id );
}

bool DeviceDescriptionsContainer::empty() const {
    return m_manufacturersNames.empty() && m_deviceDescriptions.empty() && m_deviceTypes.empty() &&
           m_deviceSubTypes.empty();
}

void DeviceDescriptionsContainer::clear() {
    m_manufacturersNames.clear();
    m_deviceDescriptions.clear();
    m_deviceTypes.clear();
    m_deviceSubTypes.clear();
}

const ManufacturersNames& DeviceDescriptionsContainer::getManufacturers() {
    return m_manufacturersNames;
}

const DeviceDescriptions& DeviceDescriptionsContainer::getDeviceDescriptioins() {
    return m_deviceDescriptions;
}

const DeviceTypesList& DeviceDescriptionsContainer::getDeviceTypesList() {
    return m_deviceTypes;
}

const DeviceSubTypesList& DeviceDescriptionsContainer::getDeviceSubTypesList() {
    return m_deviceSubTypes;
}

template < typename Container >
auto DeviceDescriptionsContainer::findOrGetDefaultValue( const Container& container,
                                                         const typename Container::key_type id ) const -> const
    typename Container::mapped_type& {
    auto iter = container.find( id );
    static typename Container::mapped_type defaultVal;
    return iter != container.end() ? iter->second : defaultVal;
}
