#include "cored/model/gateway_setting/gateway_setting_to_json.h"

#include "cored/model/cored_gateway_setting.h"
#include "cored/model/values/values.h"
#include "cored/model/resources/cored_resources.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

namespace Fields = Resources::GatewaySettingFields;

JsonObject GatewaySettingToJson::convert( const GatewaySetting& setting ) {
    auto resetState = []( GatewaySettingToJson* self ) { self->m_setting = nullptr; };
    std::unique_ptr< GatewaySettingToJson, decltype( resetState ) > scopeGuard( this, resetState );

    m_setting = &setting;
    m_joSetting = Json::object();

    m_joSetting[ Fields::Id ] = setting.getId();
    m_joSetting[ Fields::GatewayId ] = setting.getGatewayId();
    m_joSetting[ Fields::ValueType ] = setting.getValueType().toString();
    m_joSetting[ Fields::Value ] = setting.getValue().toJson();

    if( setting.getLabel().isValid() )
        m_joSetting[ Fields::Label ] = setting.getLabel().toJson();

    if( setting.getDescription().isValid() )
        m_joSetting[ Fields::Description ] = setting.getDescription().toJson();

    setting.getValue().accept( *this );

    return std::move( m_joSetting );
}

void GatewaySettingToJson::visit( const StringValue& /*value*/ ) {
    visitCommon();
}

void GatewaySettingToJson::visit( const ActionValue& /*value*/ ) {
    visitCommon();
}

void GatewaySettingToJson::visit( const TokenValue& /*value*/ ) {
    visitCommon();

    m_joSetting[ Fields::Enum ] = m_setting->getEnum().toJson();
}

void GatewaySettingToJson::visit( const IntValue& /*value*/ ) {
    visitCommon();

    if( auto minValue = m_setting->getLowerLimitValue() )
        m_joSetting[ Fields::ValueMin ] = minValue->toJson();

    if( auto maxValue = m_setting->getUpperLimitValue() )
        m_joSetting[ Fields::ValueMax ] = maxValue->toJson();
}

void GatewaySettingToJson::visitCommon() {
    if( !m_setting->getInfo().empty() )
        m_joSetting[ Fields::Info ] = m_setting->getInfo();

    m_joSetting[ Fields::SynchronizationStatus ] =
        SynchronizationStatus::toString( m_setting->getSynchronizationStatus() );

    if( auto defaultValue = m_setting->getDefaultValue() )
        m_joSetting[ Fields::ValueDefault ] = defaultValue->toJson();

    if( m_setting->isHasSetter() ) {
        m_joSetting[ Fields::HasSetter ] = m_setting->getHasSetter();
    }
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
