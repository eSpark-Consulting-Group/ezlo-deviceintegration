#include "cored/model/cored_gateway_setting.h"

#include "cored/model/gateway_setting/gateway_setting_from_json.h"
#include "cored/model/gateway_setting/gateway_setting_to_json.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/model/values/value.h"
#include "cored/model/exceptions.h"

#include "json_utils/json_utils.h"
#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

GatewaySetting::GatewaySetting( std::string id,
                                std::string gatewayId,
                                ValueType valueType,
                                std::unique_ptr< Value > value,
                                SynchronizationStatus::Enum status )
: m_id( std::move( id ) )
, m_gatewayId( std::move( gatewayId ) )
, m_valueType( std::move( valueType ) )
, m_value{ std::move( value ) }
, m_status( status ) {
    if( !m_value ) {
        throw ValueError( "invalid value" );
    }
    if( m_valueType != ValueType::Enum::Int && m_valueType != ValueType::Enum::String &&
        m_valueType != ValueType::Enum::Action && m_valueType != ValueType::Enum::Token ) {
        throw UnexpectedValueType( m_valueType );
    }
}

GatewaySetting GatewaySetting::fromJson( const JsonObject& json ) {
    GatewaySettingFromJson helper;
    return helper.convert( json );
}

JsonObject GatewaySetting::toJson() const {
    GatewaySettingToJson helper;
    return helper.convert( *this );
}

void GatewaySetting::setLabel( const TextElement& label ) {
    m_label = label;
}

void GatewaySetting::setDescription( const TextElement& description ) {
    m_description = description;
}

void GatewaySetting::setDefaultValue( std::unique_ptr< Value > value ) {
    m_defaultValue = std::move( value );
}

void GatewaySetting::setLowerLimitValue( std::unique_ptr< Value > value ) {
    m_lowerLimitValue = std::move( value );
}

void GatewaySetting::setUpperLimitValue( std::unique_ptr< Value > value ) {
    m_upperLimitValue = std::move( value );
}

void GatewaySetting::setEnum( const SettingEnum& settingEnum ) {
    m_settingEnum = settingEnum;
}

void GatewaySetting::setInfo( const Json& settingInfo ) {
    m_info = settingInfo;
}

void GatewaySetting::setSynchronizationStatus( SynchronizationStatus::Enum status ) {
    m_status = status;
}

void GatewaySetting::setHasSetter( bool hasSetter ) {
    m_hasSetter = std::make_unique< bool >( hasSetter );
}

const std::string& GatewaySetting::getId() const {
    return m_id;
}

const std::string& GatewaySetting::getGatewayId() const {
    return m_gatewayId;
}

const TextElement& GatewaySetting::getLabel() const {
    return m_label;
}

const TextElement& GatewaySetting::getDescription() const {
    return m_description;
}

const ValueType& GatewaySetting::getValueType() const {
    return m_valueType;
}

const Value& GatewaySetting::getValue() const {
    return *m_value;
}

const Value* GatewaySetting::getDefaultValue() const {
    return m_defaultValue.get();
}

const Value* GatewaySetting::getLowerLimitValue() const {
    return m_lowerLimitValue.get();
}

const Value* GatewaySetting::getUpperLimitValue() const {
    return m_upperLimitValue.get();
}

const SettingEnum& GatewaySetting::getEnum() const {
    return m_settingEnum;
}

const Json& GatewaySetting::getInfo() const {
    return m_info;
}

SynchronizationStatus::Enum GatewaySetting::getSynchronizationStatus() const {
    return m_status;
}

bool GatewaySetting::getHasSetter() const {
    return m_hasSetter && *m_hasSetter;
}

bool GatewaySetting::isHasSetter() const {
    return !!m_hasSetter;
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
