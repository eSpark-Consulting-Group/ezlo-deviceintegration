#pragma once

#include "cored/model/values/value_visitor.h"
#include "cored/model/cored_gateway_setting.h"

#include <functional>
#include <string>

namespace HubPlatform {
namespace CoreD {
namespace Model {

class Value;

class GatewaySettingFromJson : public ValueDefaultConstVisitor {
public:
    GatewaySetting convert( const JsonObject& json );

private:
    void visit( const StringValue& value ) override;
    void visit( const ActionValue& value ) override;
    void visit( const TokenValue& value ) override;
    void visit( const IntValue& value ) override;

    void visitCommon();

    std::string getId() const;
    std::string getGatewayId() const;
    SynchronizationStatus::Enum getStatus() const;
    ValueType getValueType() const;
    TextElement getTextElement( const char* key ) const;

    std::unique_ptr< Value > findValue( const char* key ) const;
    std::unique_ptr< Value > getValue( const char* key ) const;

    SettingEnum getEnum() const;
    const JsonObject& getJsSetting() const;

private:
    GatewaySetting* m_setting{ nullptr };
    const Json* m_jsSetting{ nullptr };
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
