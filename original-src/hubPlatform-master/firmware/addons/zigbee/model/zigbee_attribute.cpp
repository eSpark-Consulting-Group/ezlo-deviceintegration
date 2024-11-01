#include "zigbee/model/zigbee_attribute.h"

#include "zigbee/resources/zigbee_resources.h"
#include "zigbee/utils/attributes_formatter.h"
#include "database_client/dbqueryparser.h"
#include "json_utils/json_utils.h"

#include "common_utils/cpp14.h"

#include "zigbee/zigbee_log.h"

namespace HubPlatform {
namespace Zigbee {

std::unique_ptr< ZigbeeAttribute > ZigbeeAttributeFactory::createZigbeeAttribute( uint16_t clusterId,
                                                                                  uint16_t attributeId,
                                                                                  AttributeValueType valueType,
                                                                                  bool isMfgSpecific ) {
    Log::print(
        Log::PriorityLevel::DEBUG,
        cc::debug(
            "New Zigbee attribute created: clusterId: 0x%04x, attributeId: 0x%04x, valueType: %s, isMfgSpecific: %s" )
            .c_str(),
        clusterId,
        attributeId,
        toString( valueType ),
        isMfgSpecific ? "true" : "false" );

    return std::make_unique< ZigbeeAttribute >( attributeId, clusterId, valueType, isMfgSpecific );
}

ZigbeeAttribute::ZigbeeAttribute( uint16_t id, uint16_t clusterId, AttributeValueType type, bool isMfgSpecific )
: m_id( id )
, m_clusterId( clusterId )
, m_type( type )
, m_isMfgSpecific( isMfgSpecific )
, m_actualValue( false )
, m_value( std::string{} ) {}

ZigbeeAttribute::ZigbeeAttribute( const Json& fromJson )
: m_id( json_get_int( fromJson, Resources::Fields::Attribute::Id ) )
, m_clusterId( json_get_int( fromJson, Resources::Fields::Attribute::ClusterId ) )
, m_type( attributeValueTypeFromString( json_get_str( fromJson, Resources::Fields::Attribute::Type ) ) )
, m_isMfgSpecific( json_get_bool( fromJson, Resources::Fields::Attribute::IsMfgSpecific ) )
, m_mfgCode( json_get_int( fromJson, Resources::Fields::Attribute::MfgCode ) )
, m_actualValue( true )
, m_value( json_get_str( fromJson, Resources::Fields::Attribute::Value ) ) {}

uint16_t ZigbeeAttribute::getId() const {
    return m_id;
}

AttributeValueType ZigbeeAttribute::getType() const {
    return m_type;
}

std::string ZigbeeAttribute::getName() const {
    return toString( static_cast< ClusterId >( m_clusterId ), m_id );
}

bool ZigbeeAttribute::isMfgSpecific() const {
    return m_isMfgSpecific;
}

bool ZigbeeAttribute::hasActualValue() const {
    return m_actualValue;
}

const std::string& ZigbeeAttribute::getValue() const {
    return m_value;
}

const uint16_t ZigbeeAttribute::getMfgCode() const {
    return m_mfgCode;
}

void ZigbeeAttribute::setValue( const std::string& newValue ) {
    m_actualValue = true;
    m_value = newValue;
}

void ZigbeeAttribute::setMfgCode( const uint16_t newValue ) {
    m_mfgCode = newValue;
}

Json ZigbeeAttribute::toJson() const {
    JsonObject result;

    result[ Resources::Fields::Attribute::Id ] = m_id;
    result[ Resources::Fields::Attribute::ClusterId ] = m_clusterId;
    result[ Resources::Fields::Attribute::Name ] = getName();
    result[ Resources::Fields::Attribute::Type ] = toString( m_type );
    result[ Resources::Fields::Attribute::IsMfgSpecific ] = m_isMfgSpecific;
    result[ Resources::Fields::Attribute::MfgCode ] = m_mfgCode;
    result[ Resources::Fields::Attribute::Value ] = m_value;

    if( m_value != "" ) {
        result[ Resources::Fields::Attribute::FormattedValue ] =
            std::move( AttributesFormatter::formatAttributes( m_id, m_clusterId, m_value ) );
    }

    return std::move( result );
}

}  // namespace Zigbee
}  // namespace HubPlatform
