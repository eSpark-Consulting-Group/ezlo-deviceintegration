#include "cored/model/visitors/item_value_float_visitor.h"
#include "cored/model/values/values.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

ItemValueFloatVisitor::ItemValueFloatVisitor( const char* baseScale )
: m_baseScale( baseScale ) {}

void ItemValueFloatVisitor::visit( const HubPlatform::CoreD::Model::IntValue& intValue ) {
    clear();
    m_value = intValue.get();
}

void ItemValueFloatVisitor::visit( const HubPlatform::CoreD::Model::FloatValue& floatValue ) {
    clear();
    m_value = floatValue.get();
}

void ItemValueFloatVisitor::visit( const HubPlatform::CoreD::Model::ScalableValue& itemValue ) {
    clear();
    if( m_baseScale ) {
        m_value = itemValue.getConvertedValue( m_baseScale );
    }
    else {
        m_value = itemValue.getValue();
    }
}

float ItemValueFloatVisitor::getValue() const {
    return m_value;
}

void ItemValueFloatVisitor::clear() {
    m_value = 0;
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
