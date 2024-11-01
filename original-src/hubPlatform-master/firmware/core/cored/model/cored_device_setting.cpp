#include "cored/model/cored_device_setting.h"
#include "cored/model/device_setting/device_setting_from_json.h"
#include "cored/model/device_setting/device_setting_to_json.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/model/values/value.h"

#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

DeviceSetting::DeviceSetting( std::string deviceId, ValueType valueType, std::unique_ptr< Value > value )
: m_deviceId{ std::move( deviceId ) }
, m_valueType{ std::move( valueType ) }
, m_value{ std::move( value ) } {}

DeviceSetting::~DeviceSetting() = default;

DeviceSetting DeviceSetting::fromJson( const JsonObject& json ) {
    DeviceSettingFromJson helper;
    return helper.convert( json );
}

JsonObject DeviceSetting::toJson() const {
    DeviceSettingToJson helper;
    return helper.convert( *this );
}

void DeviceSetting::setLabel( TextElement label ) {
    m_label = std::move( label );
}

void DeviceSetting::setDescription( TextElement description ) {
    m_description = std::move( description );
}

void DeviceSetting::setDefaultValue( std::unique_ptr< Value > value ) {
    m_defaultValue = std::move( value );
}

void DeviceSetting::setLowerLimitValue( std::unique_ptr< Value > value ) {
    m_lowerLimitValue = std::move( value );
}

void DeviceSetting::setUpperLimitValue( std::unique_ptr< Value > value ) {
    m_upperLimitValue = std::move( value );
}

void DeviceSetting::setValueStep( std::unique_ptr< Value > value ) {
    m_valueStep = std::move( value );
}

void DeviceSetting::setEnum( SettingEnum settingEnum ) {
    m_settingEnum = std::move( settingEnum );
}

void DeviceSetting::setInfo( const Json& settingInfo ) {
    m_info = settingInfo;
}

void DeviceSetting::setElementsMaxNumber( ElementsNumber elementsMaxNumber ) {
    m_elementsMaxNumber = elementsMaxNumber;
}

void DeviceSetting::setSynchronizationStatus( SynchronizationStatus::Enum status ) {
    m_status = status;
}

void DeviceSetting::setHasSetter( bool hasSetter ) {
    m_hasSetter = std::make_unique< bool >( hasSetter );
}

void DeviceSetting::setTargetChannelSupported( bool targetChannelSupported ) {
    m_targetChannelSupported = std::make_unique< bool >( targetChannelSupported );
}

const std::string& DeviceSetting::getDeviceId() const {
    return m_deviceId;
}

const TextElement& DeviceSetting::getLabel() const {
    return m_label;
}

const TextElement& DeviceSetting::getDescription() const {
    return m_description;
}

const ValueType& DeviceSetting::getValueType() const {
    return m_valueType;
}

const Value& DeviceSetting::getValue() const {
    return *m_value;
}

const Value* DeviceSetting::getDefaultValue() const {
    return m_defaultValue.get();
}

const Value* DeviceSetting::getLowerLimitValue() const {
    return m_lowerLimitValue.get();
}

const Value* DeviceSetting::getUpperLimitValue() const {
    return m_upperLimitValue.get();
}

const Value* DeviceSetting::getValueStep() const {
    return m_valueStep.get();
}

const SettingEnum& DeviceSetting::getEnum() const {
    return m_settingEnum;
}

const Json& DeviceSetting::getInfo() const {
    return m_info;
}

DeviceSetting::ElementsNumber DeviceSetting::getElementsMaxNumber() const {
    return m_elementsMaxNumber;
}

SynchronizationStatus::Enum DeviceSetting::getSynchronizationStatus() const {
    return m_status;
}

common::optional< bool > DeviceSetting::getHasSetterValue() const {
    return m_hasSetter.get() ? *m_hasSetter : common::optional< bool >();
}

common::optional< bool > DeviceSetting::getTargetChannelSupportedValue() const {
    return m_targetChannelSupported.get() ? *m_targetChannelSupported : common::optional< bool >();
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
