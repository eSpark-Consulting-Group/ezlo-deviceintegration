#include "cored/model/values/int_value.h"

#include "cored/model/values/value_visitor.h"
#include "cored/model/exceptions.h"
#include "cored/model/values/value_type.h"
#include "cored/model/values/value_cast.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

std::unique_ptr< IntValue > IntValue::createValue( const Json& value ) {
    if( !value.is_number_integer() )
        throw UnexpectedJsonValueFormatError{ value, ValueType::Enum::Int };

    return std::unique_ptr< IntValue >{ new IntValue{ value.get< int >() } };
}

std::unique_ptr< IntValue > IntValue::createDefaultValue() {
    return std::unique_ptr< IntValue >{ new IntValue{ 0 } };
}

IntValue::IntValue( int value )
: Value( ValueType::Enum::Int )
, m_value{ value } {}

IntValue::~IntValue() = default;

Json IntValue::toJson() const {
    return m_value;
}

bool IntValue::isLess( const Value& other ) const {
    const auto& otherIntValue = valueCast< IntValue >( other );
    return m_value < otherIntValue.m_value;
}

bool IntValue::isEqual( const Value& other ) const {
    const auto& otherIntValue = valueCast< IntValue >( other );
    return m_value == otherIntValue.m_value;
}

void IntValue::set( const Value& other ) {
    const auto& otherIntValue = valueCast< IntValue >( other );
    m_value = otherIntValue.m_value;
}

void IntValue::accept( ValueVisitor& visitor ) {
    visitor.visit( *this );
}

void IntValue::accept( ValueConstVisitor& visitor ) const {
    visitor.visit( *this );
}

int IntValue::get() const {
    return m_value;
}

void IntValue::set( int value ) {
    m_value = value;
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
