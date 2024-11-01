#include "cored/model/device_setting/device_setting_to_json.h"
#include "cored/model/cored_device_setting.h"
#include "cored/model/values/values.h"
#include "cored/model/resources/cored_resources.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

namespace Fields = Resources::DeviceSettingFields;

DeviceSettingToJson::DeviceSettingToJson() = default;

DeviceSettingToJson::~DeviceSettingToJson() = default;

JsonObject DeviceSettingToJson::convert( const DeviceSetting& setting ) {
    auto resetState = []( DeviceSettingToJson* self ) { self->m_setting = nullptr; };
    std::unique_ptr< DeviceSettingToJson, decltype( resetState ) > scopeGuard( this, resetState );

    m_setting = &setting;
    m_joSetting = Json::object();

    m_joSetting[ Fields::DeviceId ] = setting.getDeviceId();
    m_joSetting[ Fields::ValueType ] = setting.getValueType().toString();
    m_joSetting[ Fields::Value ] = setting.getValue().toJson();

    if( setting.getLabel().isValid() )
        m_joSetting[ Fields::Label ] = setting.getLabel().toJson();

    if( setting.getDescription().isValid() )
        m_joSetting[ Fields::Description ] = setting.getDescription().toJson();

    if( !setting.getInfo().empty() )
        m_joSetting[ Fields::Info ] = setting.getInfo();

    m_joSetting[ Fields::SynchronizationStatus ] =
        SynchronizationStatus::toString( setting.getSynchronizationStatus() );

    if( auto defaultValue = setting.getDefaultValue() )
        m_joSetting[ Fields::ValueDefault ] = defaultValue->toJson();

    if( auto hasSetter = setting.getHasSetterValue() ) {
        m_joSetting[ Fields::HasSetter ] = *hasSetter;
    }

    if( auto targetChannelSupported = setting.getTargetChannelSupportedValue() ) {
        m_joSetting[ Fields::TargetChannelSupported ] = *targetChannelSupported;
    }

    setting.getValue().accept( *this );

    return std::move( m_joSetting );
}

void DeviceSettingToJson::visit( const IntValue& /*value*/ ) {
    if( auto minValue = m_setting->getLowerLimitValue() )
        m_joSetting[ Fields::ValueMin ] = minValue->toJson();

    if( auto maxValue = m_setting->getUpperLimitValue() )
        m_joSetting[ Fields::ValueMax ] = maxValue->toJson();

    if( auto valueStep = m_setting->getValueStep() )
        m_joSetting[ Fields::ValueStep ] = valueStep->toJson();
}

void DeviceSettingToJson::visit( const TokenValue& /*value*/ ) {
    m_joSetting[ Fields::Enum ] = m_setting->getEnum().toJson();
}

void DeviceSettingToJson::visit( const ScalableValue& /*value*/ ) {
    if( auto minValue = m_setting->getLowerLimitValue() )
        m_joSetting[ Fields::ValueMin ] = minValue->toJson();

    if( auto maxValue = m_setting->getUpperLimitValue() )
        m_joSetting[ Fields::ValueMax ] = maxValue->toJson();
}

void DeviceSettingToJson::visit( const DictionaryValue& /*value*/ ) {
    if( auto elementsMaxNumber = m_setting->getElementsMaxNumber() )
        m_joSetting[ Fields::ElementsMaxNumber ] = elementsMaxNumber;
}

void DeviceSettingToJson::visit( const ArrayValue& /*value*/ ) {
    m_joSetting[ Fields::Enum ] = m_setting->getEnum().toJson();
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
