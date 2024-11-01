#pragma once

#include "device_descriptions/common_types.h"
#include "base_utils/movable.h"
#include <utility>

class DeviceDescriptionsContainer final : public common::Movable {
public:
    DeviceDescriptionsContainer();
    DeviceDescriptionsContainer( ManufacturersNames&& names,
                                 DeviceDescriptions&& descriptions,
                                 DeviceTypesList&& types,
                                 ItemSubTypesList&& subTypes,
                                 DeviceSubTypesList&& deviceSubTypes );

public:
    void setDeviceManufacturers( ManufacturersNames&& names );
    void setDeviceDescriptions( DeviceDescriptions&& descriptions );
    void setDeviceTypes( DeviceTypesList&& typesList );
    void setDeviceItemSubtypes( ItemSubTypesList&& itemSubTypesList );
    void setDeviceSubtypes( DeviceSubTypesList&& subTypesList );
    void setDevicesShowAsRuleExcludedItems( DevicesShowAsRuleExcludedItems&& items );

public:
    const DeviceInfoPtr& findDeviceInfo( const DeviceId& id ) const;
    const std::string& findDeviceManufacturer( int id ) const;
    const std::string& findDeviceType( int id ) const;
    const std::string& findDeviceSubType( int id ) const;
    const std::string& findDeviceItemSubType( int id ) const;
    const std::vector< std::string >& findDeviceShowAsRuleExcludedItem( const std::string& id ) const;

public:
    bool empty() const;
    void clear();

public:
    const ManufacturersNames& getManufacturers();
    const DeviceDescriptions& getDeviceDescriptioins();
    const DeviceTypesList& getDeviceTypesList();
    const DeviceSubTypesList& getDeviceSubTypesList();

private:
    template < typename Container >
    auto findOrGetDefaultValue( const Container& container, const typename Container::key_type id ) const -> const
        typename Container::mapped_type&;

private:
    ManufacturersNames m_manufacturersNames;
    DeviceDescriptions m_deviceDescriptions;
    DeviceTypesList m_deviceTypes;
    ItemSubTypesList m_deviceItemSubTypes;
    DeviceSubTypesList m_deviceSubTypes;
    DevicesShowAsRuleExcludedItems m_devicesShowAsRuleExcludedItems;
};
