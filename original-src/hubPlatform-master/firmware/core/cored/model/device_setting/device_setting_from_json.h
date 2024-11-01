#pragma once

#include "cored/model/values/value_visitor.h"
#include "cored/model/cored_device_setting.h"

#include <functional>
#include <string>

namespace HubPlatform {
namespace CoreD {
namespace Model {

class Value;

class DeviceSettingFromJson : public ValueDefaultConstVisitor {
public:
    DeviceSettingFromJson();
    ~DeviceSettingFromJson() override;

    DeviceSetting convert( const JsonObject& json );

private:
    void visit( const IntValue& value ) override;
    void visit( const TokenValue& value ) override;
    void visit( const ScalableValue& value ) override;
    void visit( const DictionaryValue& value ) override;
    void visit( const ArrayValue& value ) override;

private:
    std::string getDeviceId() const;

    ValueType getValueType() const;

    TextElement getTextElement( const char* key ) const;

    std::unique_ptr< Value > findValue( const char* key ) const;
    std::unique_ptr< Value > getValue( const char* key ) const;

    SettingEnum getEnum() const;

    DeviceSetting::ElementsNumber getElementsMaxNumber() const;

private:
    const JsonObject& getJsSetting() const;

private:
    DeviceSetting* m_setting{ nullptr };
    const Json* m_jsSetting{ nullptr };
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
