
#include <tiny/tiny_utils.h>

#include "base_utils/utils.h"

#include "device_model/h/apiclasses.h"
#include "device_model/h/devicetype.h"

using namespace utils;

static const char g_separator = '_';

DeviceType::DeviceType()
: m_genericType( 0 )
, m_basicType( 0 )
, m_specificType( 0 )
, m_manufacturerId( 0 )
, m_manufacturerProductId( 0 )
, m_manufacturerProductType( 0 ) {}

DeviceType::DeviceType( const std::string& deviceTypeId ) {
    std::vector< std::string > parts = tiny::tools::split2vec( deviceTypeId, g_separator );
    // while parsing existing deviceTypeId assume following ID's order
    // m_genericType, m_basicType, m_specificType, m_manufacturerId, m_manufacturerProductType, m_manufacturerProductId

    // pad with zeros non-existing parts to prevent fails
    while( parts.size() < 6 )
        parts.resize( 6, "0" );

    m_genericType = atoi( parts[ 0 ].c_str() );
    m_basicType = atoi( parts[ 1 ].c_str() );
    m_specificType = atoi( parts[ 2 ].c_str() );
    m_manufacturerId = atoi( parts[ 3 ].c_str() );
    m_manufacturerProductType = atoi( parts[ 4 ].c_str() );
    m_manufacturerProductId = atoi( parts[ 5 ].c_str() );
}

DeviceType::DeviceType( int genericType,
                        int basicType,
                        int specificType,
                        int manufacturerId,
                        int manufacturerProductType,
                        int manufacturerProductId )
: m_genericType( genericType )
, m_basicType( basicType )
, m_specificType( specificType )
, m_manufacturerId( manufacturerId )
, m_manufacturerProductType( manufacturerProductType )
, m_manufacturerProductId( manufacturerProductId ) {}

std::string DeviceType::getId() const {
    return ( std::to_string( m_genericType ) + g_separator + std::to_string( m_basicType ) + g_separator +
             std::to_string( m_specificType ) + g_separator + std::to_string( m_manufacturerId ) + g_separator +
             std::to_string( m_manufacturerProductType ) + g_separator + std::to_string( m_manufacturerProductId ) );
}

int DeviceType::getGenericType() const {
    return m_genericType;
}

int DeviceType::getBasicType() const {
    return m_basicType;
}

int DeviceType::getSpecificType() const {
    return m_specificType;
}

int DeviceType::getManufacturerId() const {
    return m_manufacturerId;
}

int DeviceType::getManufacturerProductType() const {
    return m_manufacturerProductType;
}

int DeviceType::getManufacturerProductId() const {
    return m_manufacturerProductId;
}
