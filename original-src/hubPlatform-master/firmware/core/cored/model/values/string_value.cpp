#include "cored/model/values/string_value.h"

#include "cored/model/values/value_visitor.h"
#include "cored/model/exceptions.h"
#include "cored/model/values/value_type.h"
#include "cored/model/values/value_cast.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

std::unique_ptr< StringValue > StringValue::createValue( const Json& value ) {
    if( !value.is_string() )
        throw UnexpectedJsonValueFormatError{ value, ValueType::Enum::String };

    return std::unique_ptr< StringValue >{ new StringValue{ value.get< std::string >() } };
}

std::unique_ptr< StringValue > StringValue::createDefaultValue() {
    return std::unique_ptr< StringValue >{ new StringValue{} };
}

StringValue::StringValue()
: Value( ValueType::Enum::String ) {}

StringValue::StringValue( std::string&& value )
: Value( ValueType::Enum::String )
, m_value{ std::move( value ) } {}

StringValue::~StringValue() = default;

Json StringValue::toJson() const {
    return m_value;
}

bool StringValue::isLess( const Value& other ) const {
    const auto& otherStringValue = valueCast< StringValue >( other );
    return m_value < otherStringValue.m_value;
}

bool StringValue::isEqual( const Value& other ) const {
    const auto& otherStringValue = valueCast< StringValue >( other );
    return m_value == otherStringValue.m_value;
}

void StringValue::set( const Value& other ) {
    const auto& otherStringValue = valueCast< StringValue >( other );
    m_value = otherStringValue.m_value;
}

void StringValue::accept( ValueVisitor& visitor ) {
    visitor.visit( *this );
}

void StringValue::accept( ValueConstVisitor& visitor ) const {
    visitor.visit( *this );
}

const std::string& StringValue::get() const {
    return m_value;
}

void StringValue::set( const std::string& value ) {
    m_value = value;
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
