#include "cored/model/visitors/item_value_string_visitor.h"
#include "cored/model/values/values.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

ItemValueStringVisitor::ItemValueStringVisitor() {}

void ItemValueStringVisitor::visit( const HubPlatform::CoreD::Model::StringValue& stringValue ) {
    clear();
    m_value = stringValue.get();
}

void ItemValueStringVisitor::visit( const HubPlatform::CoreD::Model::TokenValue& tokenValue ) {
    clear();
    m_value = tokenValue.get();
}

std::string ItemValueStringVisitor::getValue() const {
    return m_value;
}

void ItemValueStringVisitor::clear() {
    m_value.clear();
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
