#include "cored/model/gateway_setting/gateway_setting_from_json.h"

#include "cored/model/setting/setting_enum.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/model/values/values.h"
#include "cored/model/values/value_factory.h"
#include "cored/model/exceptions.h"

#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

namespace Fields = Resources::GatewaySettingFields;

GatewaySetting GatewaySettingFromJson::convert( const JsonObject& json ) {
    auto resetState = []( GatewaySettingFromJson* self ) {
        self->m_setting = nullptr;
        self->m_jsSetting = nullptr;
    };
    std::unique_ptr< GatewaySettingFromJson, decltype( resetState ) > scopeGuard( this, resetState );

    m_jsSetting = &json;

    GatewaySetting setting{ getId(), getGatewayId(), getValueType(), getValue( Fields::Value ), getStatus() };
    setting.setLabel( getTextElement( Fields::Label ) );
    setting.setDescription( getTextElement( Fields::Description ) );

    m_setting = &setting;

    setting.getValue().accept( *this );

    return setting;
}

void GatewaySettingFromJson::visit( const StringValue& /*value*/ ) {
    visitCommon();
}

void GatewaySettingFromJson::visit( const ActionValue& /*value*/ ) {
    visitCommon();
}

void GatewaySettingFromJson::visit( const TokenValue& /*value*/ ) {
    visitCommon();

    m_setting->setEnum( getEnum() );
}
void GatewaySettingFromJson::visit( const IntValue& /*value*/ ) {
    visitCommon();

    m_setting->setLowerLimitValue( findValue( Fields::ValueMin ) );
    m_setting->setUpperLimitValue( findValue( Fields::ValueMax ) );
}

void GatewaySettingFromJson::visitCommon() {
    m_setting->setInfo( getJsSetting()[ Resources::GatewaySettingFields::Info ] );

    m_setting->setDefaultValue( findValue( Fields::ValueDefault ) );

    const auto statusStr = json_get_str( getJsSetting(), Resources::GatewaySettingFields::SynchronizationStatus );
    if( !statusStr.empty() ) {
        m_setting->setSynchronizationStatus( SynchronizationStatus::fromString( statusStr ) );
    }

    bool hasSetter;
    if( json_get( getJsSetting(), Resources::GatewaySettingFields::HasSetter, hasSetter ) ) {
        m_setting->setHasSetter( hasSetter );
    }
}

std::string GatewaySettingFromJson::getId() const {
    auto id = json_get_str( getJsSetting(), Resources::GatewaySettingFields::Id );
    if( id.empty() ) {
        throw CoreDModelError( "Invalid gateway setting id" );
    }

    return id;
}

std::string GatewaySettingFromJson::getGatewayId() const {
    auto gatewayId = json_get_str( getJsSetting(), Resources::GatewaySettingFields::GatewayId );
    if( gatewayId.empty() )
        throw CoreDModelError( "Invalid gateway id" );

    return gatewayId;
}

ValueType GatewaySettingFromJson::getValueType() const {
    auto valueTypeStr = json_get_str( getJsSetting(), Resources::GatewaySettingFields::ValueType );
    const auto valueType = ValueType::fromString( valueTypeStr );
    if( valueType == ValueType::Enum::NoValue )
        throw CoreDModelError{ "Invalid value type: " + valueTypeStr };

    return valueType;
}

SynchronizationStatus::Enum GatewaySettingFromJson::getStatus() const {
    return SynchronizationStatus::fromString(
        json_get_str( getJsSetting(), Resources::GatewaySettingFields::SynchronizationStatus ) );
}

TextElement GatewaySettingFromJson::getTextElement( const char* key ) const {
    auto it = getJsSetting().find( key );
    if( it == getJsSetting().end() )
        return {};

    return TextElement::fromJson( *it );
}

std::unique_ptr< Value > GatewaySettingFromJson::findValue( const char* key ) const {
    auto it = getJsSetting().find( key );
    if( it != getJsSetting().end() )
        return ValueFactory::createItemValue( getValueType(), *it );

    return {};
}

std::unique_ptr< Value > GatewaySettingFromJson::getValue( const char* key ) const {
    return ValueFactory::createItemValue( getValueType(), getJsSetting()[ key ] );
}

SettingEnum GatewaySettingFromJson::getEnum() const {
    return SettingEnum::fromJson( getJsSetting()[ Resources::GatewaySettingFields::Enum ] );
}

const JsonObject& GatewaySettingFromJson::getJsSetting() const {
    return *m_jsSetting;
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
