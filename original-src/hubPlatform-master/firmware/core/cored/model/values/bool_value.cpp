#include "cored/model/values/bool_value.h"

#include "cored/model/exceptions.h"
#include "cored/model/values/value_visitor.h"
#include "cored/model/values/value_type.h"
#include "cored/model/values/value_cast.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

std::unique_ptr< BoolValue > BoolValue::createValue( const Json& value ) {
    if( value.is_boolean() )
        return std::unique_ptr< BoolValue >{ new BoolValue{ value.get< bool >() } };

    if( value.is_number_integer() )
        return std::unique_ptr< BoolValue >{ new BoolValue{ static_cast< bool >( value.get< int >() ) } };

    throw UnexpectedJsonValueFormatError{ value, ValueType::Enum::Bool };
}

std::unique_ptr< BoolValue > BoolValue::createDefaultValue() {
    return std::unique_ptr< BoolValue >{ new BoolValue{ false } };
}

BoolValue::BoolValue( bool value )
: Value( ValueType::Enum::Bool )
, m_value{ value } {}

BoolValue::~BoolValue() = default;

Json BoolValue::toJson() const {
    return m_value;
}

bool BoolValue::isEqual( const Value& other ) const {
    const auto& otherBoolValue = valueCast< BoolValue >( other );
    return m_value == otherBoolValue.m_value;
}

void BoolValue::set( const Value& other ) {
    const auto& otherBoolValue = valueCast< BoolValue >( other );
    m_value = otherBoolValue.m_value;
}

void BoolValue::accept( ValueVisitor& visitor ) {
    visitor.visit( *this );
}

void BoolValue::accept( ValueConstVisitor& visitor ) const {
    visitor.visit( *this );
}

bool BoolValue::get() const {
    return m_value;
}

void BoolValue::set( bool value ) {
    m_value = value;
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
