#pragma once

#include "cored/model/values/value_type.h"
#include "cored/model/setting/setting_enum.h"
#include "cored/model/setting/text_element.h"
#include "cored/model/setting/synchronization_status.h"

#include "json_utils/json_fwd.h"
#include "base_utils/optional.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

class Value;

class DeviceSetting {
public:
    DeviceSetting( std::string deviceId, ValueType valueType, std::unique_ptr< Value > value );

    ~DeviceSetting();

    DeviceSetting( DeviceSetting&& ) = default;

    DeviceSetting& operator=( DeviceSetting&& ) = default;

public:
    static DeviceSetting fromJson( const JsonObject& json );

    JsonObject toJson() const;

public:
    void setLabel( TextElement label );

    void setDescription( TextElement description );

    void setDefaultValue( std::unique_ptr< Value > value );

    void setLowerLimitValue( std::unique_ptr< Value > value );

    void setUpperLimitValue( std::unique_ptr< Value > value );

    void setValueStep( std::unique_ptr< Value > value );

    void setEnum( SettingEnum settingEnum );

    void setInfo( const Json& settingInfo );

    using ElementsNumber = std::size_t;
    void setElementsMaxNumber( ElementsNumber elementsMaxNumber );

    void setSynchronizationStatus( SynchronizationStatus::Enum status );

    void setHasSetter( bool hasSetter );

    void setTargetChannelSupported( bool targetChannelSupported );

public:
    const std::string& getDeviceId() const;

    const TextElement& getLabel() const;

    const TextElement& getDescription() const;

    const ValueType& getValueType() const;

    const Value& getValue() const;

    const Value* getDefaultValue() const;

    const Value* getLowerLimitValue() const;

    const Value* getUpperLimitValue() const;

    const Value* getValueStep() const;

    const SettingEnum& getEnum() const;

    const Json& getInfo() const;

    ElementsNumber getElementsMaxNumber() const;

    SynchronizationStatus::Enum getSynchronizationStatus() const;

    common::optional< bool > getHasSetterValue() const;

    common::optional< bool > getTargetChannelSupportedValue() const;

private:
    std::string m_deviceId;

    TextElement m_label;

    TextElement m_description;

    ValueType m_valueType;

    std::unique_ptr< Value > m_value;

    std::unique_ptr< Value > m_defaultValue;

    std::unique_ptr< Value > m_lowerLimitValue;

    std::unique_ptr< Value > m_upperLimitValue;

    std::unique_ptr< Value > m_valueStep;

    SettingEnum m_settingEnum;

    Json m_info;

    ElementsNumber m_elementsMaxNumber{ 0 };

    SynchronizationStatus::Enum m_status{ SynchronizationStatus::Synchronized };
    std::unique_ptr< bool > m_hasSetter;
    std::unique_ptr< bool > m_targetChannelSupported;
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
