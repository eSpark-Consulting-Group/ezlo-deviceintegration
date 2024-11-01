#pragma once

#include "cored/model/values/value_visitor.h"

#include "json_utils/json_fwd.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

class DeviceSetting;

class DeviceSettingToJson : public ValueDefaultConstVisitor {
public:
    DeviceSettingToJson();
    ~DeviceSettingToJson() override;

    JsonObject convert( const DeviceSetting& setting );

private:
    void visit( const IntValue& value ) override;
    void visit( const TokenValue& value ) override;
    void visit( const ScalableValue& value ) override;
    void visit( const DictionaryValue& value ) override;
    void visit( const ArrayValue& value ) override;

private:
    const DeviceSetting* m_setting{ nullptr };
    JsonObject m_joSetting;
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
