#pragma once

#include <type_traits>
#include "zwave_definitions.h"

class DeviceId final {
public:
    using ManufacturersEnumType = std::underlying_type< ManufacturerId >::type;

public:
    DeviceId( const ManufacturerId manufacturerId,
              const unsigned int manufacturerProductType,
              const unsigned int manufacturerProductId )
    : m_manufacturerId( static_cast< const ManufacturerId >( manufacturerId ) )
    , m_manufacturerProductType( manufacturerProductType )
    , m_manufacturerProductId( manufacturerProductId ) {}

    DeviceId( const ManufacturersEnumType manufacturerId,
              const int manufacturerProductType,
              const int manufacturerProductId )
    : m_manufacturerId( static_cast< const ManufacturerId >( manufacturerId ) )
    , m_manufacturerProductType( static_cast< unsigned int >( manufacturerProductType ) )
    , m_manufacturerProductId( static_cast< unsigned int >( manufacturerProductId ) ) {}
    ManufacturerId manufacturerId() const {
        return m_manufacturerId;
    }
    unsigned int manufacturerProductType() const {
        return m_manufacturerProductType;
    }
    unsigned int manufacturerProductId() const {
        return m_manufacturerProductId;
    }

    bool operator<( const DeviceId& deviceId ) const {
        return ( m_manufacturerId < deviceId.m_manufacturerId ||
                 ( m_manufacturerId == deviceId.m_manufacturerId &&
                   ( m_manufacturerProductType < deviceId.m_manufacturerProductType ||
                     ( m_manufacturerProductType == deviceId.m_manufacturerProductType &&
                       m_manufacturerProductId < deviceId.m_manufacturerProductId ) ) ) );
    }

private:
    ManufacturerId m_manufacturerId;
    unsigned int m_manufacturerProductType = 0;
    unsigned int m_manufacturerProductId = 0;
};
