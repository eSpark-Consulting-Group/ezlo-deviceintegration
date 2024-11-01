#include "cored/model/values/float_value.h"

#include "cored/model/values/value_visitor.h"
#include "cored/model/exceptions.h"
#include "cored/model/values/value_type.h"
#include "cored/model/values/value_cast.h"

#include "base_utils/float_operations.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

std::unique_ptr< FloatValue > FloatValue::createValue( const Json& value ) {
    if( !value.is_number() )
        throw UnexpectedJsonValueFormatError{ value, ValueType::Enum::Float };

    return std::unique_ptr< FloatValue >{ new FloatValue{ value.get< float >() } };
}

std::unique_ptr< FloatValue > FloatValue::createDefaultValue() {
    return std::unique_ptr< FloatValue >{ new FloatValue{ 0.0f } };
}

FloatValue::FloatValue( float value )
: Value( ValueType::Enum::Float )
, m_value{ value } {}

FloatValue::~FloatValue() = default;

Json FloatValue::toJson() const {
    return m_value;
}

bool FloatValue::isLess( const Value& other ) const {
    const auto& otherValue = valueCast< FloatValue >( other );
    return isFloatLess( m_value, otherValue.m_value );
}

bool FloatValue::isEqual( const Value& value ) const {
    const auto& otherFloatValue = valueCast< FloatValue >( value );
    return areFloatsEqual( m_value, otherFloatValue.m_value );
}

void FloatValue::set( const Value& value ) {
    const auto& otherFloatValue = valueCast< FloatValue >( value );
    m_value = otherFloatValue.m_value;
}

void FloatValue::accept( ValueVisitor& visitor ) {
    visitor.visit( *this );
}

void FloatValue::accept( ValueConstVisitor& visitor ) const {
    visitor.visit( *this );
}

float FloatValue::get() const {
    return m_value;
}

void FloatValue::set( float value ) {
    m_value = value;
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
