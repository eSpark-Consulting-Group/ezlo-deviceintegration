#pragma once

#include "cored/model/values/value_type.h"
#include "cored/model/setting/setting_enum.h"
#include "cored/model/setting/text_element.h"
#include "cored/model/setting/synchronization_status.h"

#include "json_utils/json_fwd.h"

#include <memory>
#include <exception>

namespace HubPlatform {
namespace CoreD {
namespace Model {

class Value;

class GatewaySetting final {
public:
    GatewaySetting( std::string id,
                    std::string gatewayId,
                    ValueType valueType,
                    std::unique_ptr< Value > value,
                    SynchronizationStatus::Enum status );

    static GatewaySetting fromJson( const JsonObject& json );
    JsonObject toJson() const;

    void setLabel( const TextElement& label );
    void setDescription( const TextElement& description );
    void setDefaultValue( std::unique_ptr< Value > value );
    void setLowerLimitValue( std::unique_ptr< Value > value );
    void setUpperLimitValue( std::unique_ptr< Value > value );
    void setEnum( const SettingEnum& settingEnum );
    void setInfo( const Json& settingInfo );
    void setSynchronizationStatus( SynchronizationStatus::Enum status );
    void setHasSetter( bool hasSetter );

    const std::string& getId() const;
    const std::string& getGatewayId() const;
    const TextElement& getLabel() const;
    const TextElement& getDescription() const;
    const ValueType& getValueType() const;
    const Value& getValue() const;
    const Value* getDefaultValue() const;
    const Value* getLowerLimitValue() const;
    const Value* getUpperLimitValue() const;
    const SettingEnum& getEnum() const;
    const Json& getInfo() const;
    SynchronizationStatus::Enum getSynchronizationStatus() const;
    bool getHasSetter() const;
    bool isHasSetter() const;

private:
    std::string m_id;
    std::string m_gatewayId;
    TextElement m_label;
    TextElement m_description;
    ValueType m_valueType;
    std::unique_ptr< Value > m_value;
    std::unique_ptr< Value > m_defaultValue;
    std::unique_ptr< Value > m_lowerLimitValue;
    std::unique_ptr< Value > m_upperLimitValue;
    SettingEnum m_settingEnum;
    Json m_info;
    SynchronizationStatus::Enum m_status{ SynchronizationStatus::Synchronized };
    std::unique_ptr< bool > m_hasSetter;
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
