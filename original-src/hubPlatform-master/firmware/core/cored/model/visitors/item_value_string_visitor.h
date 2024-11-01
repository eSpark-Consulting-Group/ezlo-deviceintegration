#pragma once

#include "cored/model/values/value_visitor.h"
#include <string>

namespace HubPlatform {
namespace CoreD {
namespace Model {

class ItemValueStringVisitor : public CoreD::Model::ValueDefaultConstVisitor {
public:
    ItemValueStringVisitor();

    void visit( const HubPlatform::CoreD::Model::StringValue& stringValue ) override;
    void visit( const HubPlatform::CoreD::Model::TokenValue& tokenValue ) override;
    std::string getValue() const;
    void clear();

private:
    std::string m_value;
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
