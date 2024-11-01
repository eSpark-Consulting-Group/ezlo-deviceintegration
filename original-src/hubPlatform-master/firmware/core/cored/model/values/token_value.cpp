#include "cored/model/values/token_value.h"

#include "cored/model/values/value_visitor.h"
#include "cored/model/exceptions.h"
#include "cored/model/values/value_type.h"
#include "cored/model/values/value_cast.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

std::unique_ptr< TokenValue > TokenValue::createValue( const Json& value ) {
    if( !value.is_string() )
        throw UnexpectedJsonValueFormatError{ value, ValueType::Enum::Token };

    return std::unique_ptr< TokenValue >{ new TokenValue{ value.get< std::string >() } };
}

std::unique_ptr< TokenValue > TokenValue::createDefaultValue() {
    return std::unique_ptr< TokenValue >{ new TokenValue{} };
}

TokenValue::TokenValue()
: Value( ValueType::Enum::Token ) {}

TokenValue::TokenValue( std::string&& value )
: Value( ValueType::Enum::Token )
, m_value{ std::move( value ) } {}

TokenValue::~TokenValue() = default;

Json TokenValue::toJson() const {
    return m_value;
}

bool TokenValue::isEqual( const Value& other ) const {
    const auto& otherTokenValue = valueCast< TokenValue >( other );
    return m_value == otherTokenValue.m_value;
}

void TokenValue::set( const Value& other ) {
    const auto& otherTokenValue = valueCast< TokenValue >( other );
    m_value = otherTokenValue.m_value;
}

void TokenValue::accept( ValueVisitor& visitor ) {
    visitor.visit( *this );
}

void TokenValue::accept( ValueConstVisitor& visitor ) const {
    visitor.visit( *this );
}

const std::string& TokenValue::get() const {
    return m_value;
}

void TokenValue::set( const std::string& value ) {
    m_value = value;
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
