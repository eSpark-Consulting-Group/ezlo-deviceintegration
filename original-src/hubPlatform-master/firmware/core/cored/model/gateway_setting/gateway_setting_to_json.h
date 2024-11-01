#pragma once

#include "cored/model/values/value_visitor.h"

#include "json_utils/json_fwd.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

class GatewaySetting;

class GatewaySettingToJson : public ValueDefaultConstVisitor {
public:
    JsonObject convert( const GatewaySetting& setting );

private:
    void visit( const StringValue& value ) override;
    void visit( const ActionValue& value ) override;
    void visit( const TokenValue& value ) override;
    void visit( const IntValue& value ) override;

    void visitCommon();

private:
    const GatewaySetting* m_setting{ nullptr };
    JsonObject m_joSetting;
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
