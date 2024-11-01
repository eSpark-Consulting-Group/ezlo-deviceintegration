#include "cored/model/values/zwave_configuration_value.h"
#include "cored/model/exceptions.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/model/values/value_visitor.h"
#include "cored/model/values/value_cast.h"

#include "json_utils/json_custom_getters.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

std::unique_ptr< ZwaveConfigurationValue > ZwaveConfigurationValue::createValue( const Json& value ) {
    using namespace JsonCustomGetters;
    namespace Fields = Resources::ZwaveConfigurationValueFields;

    auto configValue = createDefaultValue();

    const auto getter = makeStructGetter(
        makeMemberMapping( &ZwaveConfigurationValue::m_name, Fields::Name ),
        makeMemberMapping( &ZwaveConfigurationValue::m_description, Fields::Description ),
        makeMemberMapping( &ZwaveConfigurationValue::m_number, Fields::Number ),
        makeMemberMapping( &ZwaveConfigurationValue::m_size, Fields::Size ),
        makeMemberMapping( &ZwaveConfigurationValue::m_value, Fields::Value, OptionalMemberPolicy() ),
        makeMemberMapping( &ZwaveConfigurationValue::m_hasSetter, Fields::HasSetter, OptionalMemberPolicy( false ) ) );

    if( !getter( value, *configValue ) )
        throw UnexpectedJsonValueFormatError{ value, ValueType::Enum::ZwaveConfiguration };

    configValue->m_format = getFormat( value );

    const auto statusStr = json_get_str( value, Resources::DeviceSettingFields::SynchronizationStatus );
    if( !statusStr.empty() ) {
        configValue->m_status = SynchronizationStatus::fromString( statusStr );
    }

    return configValue;
}

std::unique_ptr< ZwaveConfigurationValue > ZwaveConfigurationValue::createDefaultValue() {
    return std::unique_ptr< ZwaveConfigurationValue >{ new ZwaveConfigurationValue{
        ValueType::Enum::ZwaveConfiguration } };
}

ZwaveConfigurationValue::~ZwaveConfigurationValue() = default;

Json ZwaveConfigurationValue::toJson() const {
    namespace Fields = Resources::ZwaveConfigurationValueFields;

    return JSON( Fields::Name << m_name << Fields::Description << m_description << Fields::Number << m_number
                              << Fields::Size << m_size << Fields::Value << m_value << Fields::Format
                              << formatToString( m_format ) << Resources::DeviceSettingFields::SynchronizationStatus
                              << SynchronizationStatus::toString( m_status ) << Fields::HasSetter << m_hasSetter );
}

void ZwaveConfigurationValue::accept( ValueVisitor& visitor ) {
    visitor.visit( *this );
}

void ZwaveConfigurationValue::accept( ValueConstVisitor& visitor ) const {
    visitor.visit( *this );
}

bool ZwaveConfigurationValue::isEqual( const Value& other ) const {
    const auto& configValue = valueCast< ZwaveConfigurationValue >( other );

    return std::tie( m_name, m_description, m_number, m_size, m_value, m_format, m_status, m_hasSetter ) ==
           std::tie( configValue.m_name,
                     configValue.m_description,
                     configValue.m_number,
                     configValue.m_size,
                     configValue.m_value,
                     configValue.m_format,
                     configValue.m_status,
                     configValue.m_hasSetter );
}

void ZwaveConfigurationValue::set( const Value& other ) {
    const auto& configValue = valueCast< ZwaveConfigurationValue >( other );

    m_name = configValue.m_name;
    m_description = configValue.m_description;
    m_number = configValue.m_number;
    m_size = configValue.m_size;
    m_value = configValue.m_value;
    m_format = configValue.m_format;
    m_status = configValue.m_status;
    m_hasSetter = configValue.m_hasSetter;
}

ZwaveConfigurationValue::Format ZwaveConfigurationValue::getFormat( const Json& value ) {
    auto formatStr = json_get_str( value, Resources::ZwaveConfigurationValueFields::Format );

    if( formatStr == "dec" )
        return Format::Decimal;
    if( formatStr == "hex" )
        return Format::Hexadecimal;

    throw UnexpectedJsonValueFormatError{ value, ValueType::Enum::ZwaveConfiguration };
}

std::string ZwaveConfigurationValue::formatToString( Format value ) {
    switch( value ) {
    case Format::Decimal:
        return "dec";
    case Format::Hexadecimal:
        return "hex";
    default:
        return "";
    }
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
