#include "cored/model/device_setting/device_setting_from_json.h"
#include "cored/model/setting/setting_enum.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/model/values/values.h"
#include "cored/model/values/value_factory.h"
#include "cored/model/exceptions.h"

#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

namespace Fields = Resources::DeviceSettingFields;

DeviceSettingFromJson::DeviceSettingFromJson() = default;

DeviceSettingFromJson::~DeviceSettingFromJson() = default;

DeviceSetting DeviceSettingFromJson::convert( const JsonObject& json ) {
    auto resetState = []( DeviceSettingFromJson* self ) {
        self->m_setting = nullptr;
        self->m_jsSetting = nullptr;
    };
    std::unique_ptr< DeviceSettingFromJson, decltype( resetState ) > scopeGuard( this, resetState );

    m_jsSetting = &json;
    DeviceSetting setting{ getDeviceId(), getValueType(), getValue( Fields::Value ) };

    m_setting = &setting;

    setting.setLabel( getTextElement( Fields::Label ) );
    setting.setDescription( getTextElement( Fields::Description ) );

    setting.setInfo( json[ Fields::Info ] );

    setting.setDefaultValue( findValue( Fields::ValueDefault ) );

    const auto statusStr = json_get_str( json, Fields::SynchronizationStatus );
    if( !statusStr.empty() ) {
        setting.setSynchronizationStatus( SynchronizationStatus::fromString( statusStr ) );
    }

    bool hasSetter;
    if( json_get( getJsSetting(), Fields::HasSetter, hasSetter ) ) {
        setting.setHasSetter( hasSetter );
    }

    bool targetChannelSupported;
    if( json_get( getJsSetting(), Fields::TargetChannelSupported, targetChannelSupported ) ) {
        setting.setTargetChannelSupported( targetChannelSupported );
    }

    setting.getValue().accept( *this );

    return setting;
}

void DeviceSettingFromJson::visit( const IntValue& /*value*/ ) {
    m_setting->setLowerLimitValue( findValue( Fields::ValueMin ) );
    m_setting->setUpperLimitValue( findValue( Fields::ValueMax ) );
    m_setting->setValueStep( findValue( Fields::ValueStep ) );
}

void DeviceSettingFromJson::visit( const TokenValue& /*value*/ ) {
    m_setting->setEnum( getEnum() );
}

void DeviceSettingFromJson::visit( const ScalableValue& /*value*/ ) {
    m_setting->setLowerLimitValue( findValue( Fields::ValueMin ) );
    m_setting->setUpperLimitValue( findValue( Fields::ValueMax ) );
}

void DeviceSettingFromJson::visit( const DictionaryValue& /*value*/ ) {
    if( auto elementsMaxNumber = getElementsMaxNumber() )
        m_setting->setElementsMaxNumber( elementsMaxNumber );
}

void DeviceSettingFromJson::visit( const ArrayValue& /*value*/ ) {
    m_setting->setEnum( getEnum() );
}

std::string DeviceSettingFromJson::getDeviceId() const {
    auto deviceId = json_get_str( getJsSetting(), Fields::DeviceId );
    if( deviceId.empty() )
        throw CoreDModelError( "Invalid device id" );

    return deviceId;
}

ValueType DeviceSettingFromJson::getValueType() const {
    auto valueTypeStr = json_get_str( getJsSetting(), Fields::ValueType );
    const auto valueType = ValueType::fromString( valueTypeStr );
    if( valueType == ValueType::Enum::NoValue )
        throw CoreDModelError{ "Invalid value type: " + valueTypeStr };

    return valueType;
}

TextElement DeviceSettingFromJson::getTextElement( const char* key ) const {
    auto it = getJsSetting().find( key );
    if( it == getJsSetting().end() )
        return {};

    return TextElement::fromJson( *it );
}

std::unique_ptr< Value > DeviceSettingFromJson::findValue( const char* key ) const {
    auto it = getJsSetting().find( key );
    if( it != getJsSetting().end() )
        return ValueFactory::createItemValue( getValueType(), *it );

    return {};
}

std::unique_ptr< Value > DeviceSettingFromJson::getValue( const char* key ) const {
    return ValueFactory::createItemValue( getValueType(), getJsSetting()[ key ] );
}

SettingEnum DeviceSettingFromJson::getEnum() const {
    return SettingEnum::fromJson( getJsSetting()[ Fields::Enum ] );
}

DeviceSetting::ElementsNumber DeviceSettingFromJson::getElementsMaxNumber() const {
    auto jsMaxNumber = getJsSetting().find( Fields::ElementsMaxNumber );
    if( jsMaxNumber == getJsSetting().end() )
        return 0;

    if( !jsMaxNumber->is_number_integer() )
        throw CoreDModelError{ "Invalid elements max number: " + jsMaxNumber->dump() };

    return jsMaxNumber->get< int >();
}

const JsonObject& DeviceSettingFromJson::getJsSetting() const {
    return *m_jsSetting;
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
