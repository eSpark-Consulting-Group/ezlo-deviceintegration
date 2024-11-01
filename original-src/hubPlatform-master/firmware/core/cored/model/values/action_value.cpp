#include "cored/model/values/action_value.h"
#include "cored/model/exceptions.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/model/values/value_visitor.h"
#include "cored/model/values/value_cast.h"

#include "json_utils/json_custom_getters.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

std::unique_ptr< ActionValue > ActionValue::createValue( const Json& value ) {
    using namespace JsonCustomGetters;

    auto actionValue = createDefaultValue();

    try {
        actionValue->m_value = TextElement::fromJson( value );
    }
    catch( const CoreDModelError& ) {
        throw UnexpectedJsonValueFormatError{ value, ValueType::Enum::Action };
    }

    return actionValue;
}

std::unique_ptr< ActionValue > ActionValue::createDefaultValue() {
    return std::unique_ptr< ActionValue >{ new ActionValue{ ValueType::Enum::Action } };
}

ActionValue::~ActionValue() = default;

Json ActionValue::toJson() const {
    return m_value.toJson();
}

void ActionValue::accept( ValueVisitor& visitor ) {
    visitor.visit( *this );
}

void ActionValue::accept( ValueConstVisitor& visitor ) const {
    visitor.visit( *this );
}

bool ActionValue::isEqual( const Value& other ) const {
    const auto& actionValue = valueCast< ActionValue >( other );

    return m_value == actionValue.m_value;
}

void ActionValue::set( const Value& other ) {
    const auto& actionValue = valueCast< ActionValue >( other );

    m_value = actionValue.m_value;
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
