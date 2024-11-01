#include "cored/model/values/button_state_value.h"

#include "cored/model/exceptions.h"
#include "cored/model/values/value_visitor.h"
#include "cored/model/values/value_type.h"
#include "cored/model/values/value_cast.h"

#include "cored/model/resources/cored_resources.h"

#include "json_utils/json_wrapper.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

std::unique_ptr< ButtonStateValue > ButtonStateValue::createValue( const Json& value ) {
    using namespace Resources::ButtonStateValueFields;

    auto buttonIdIt = value.find( ButtonIdFieldName );
    if( buttonIdIt == value.end() || !buttonIdIt->is_number_integer() ) {
        throw UnexpectedJsonValueFormatError{ value, ValueType::Enum::ButtonState };
    }

    auto buttonStateIt = value.find( ButtonStateFieldName );
    if( buttonStateIt == value.end() || !buttonStateIt->is_string() ) {
        throw UnexpectedJsonValueFormatError{ value, ValueType::Enum::ButtonState };
    }

    return std::unique_ptr< ButtonStateValue >(
        new ButtonStateValue( buttonIdIt->get< int >(), buttonStateIt->get< std::string >() ) );
}

std::unique_ptr< ButtonStateValue > ButtonStateValue::createDefaultValue() {
    return std::unique_ptr< ButtonStateValue >{ new ButtonStateValue{ 0, "" } };
}

ButtonStateValue::ButtonStateValue( int buttonId, std::string buttonState )
: Value( ValueType::Enum::ButtonState )
, m_buttonId( buttonId )
, m_buttonState( std::move( buttonState ) ) {}

ButtonStateValue::~ButtonStateValue() = default;

Json ButtonStateValue::toJson() const {
    using namespace Resources;
    return JSON( ButtonStateValueFields::ButtonIdFieldName << m_buttonId << ButtonStateValueFields::ButtonStateFieldName
                                                           << m_buttonState );
}

bool ButtonStateValue::isEqual( const Value& other ) const {
    const auto& otherButtonStateValue = valueCast< ButtonStateValue >( other );
    return m_buttonId == otherButtonStateValue.m_buttonId && m_buttonState == otherButtonStateValue.m_buttonState;
}

void ButtonStateValue::set( const Value& other ) {
    const auto& otherButtonStateValue = valueCast< ButtonStateValue >( other );
    m_buttonId = otherButtonStateValue.m_buttonId;
    m_buttonState = otherButtonStateValue.m_buttonState;
}

void ButtonStateValue::accept( ValueVisitor& visitor ) {
    visitor.visit( *this );
}

void ButtonStateValue::accept( ValueConstVisitor& visitor ) const {
    visitor.visit( *this );
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
