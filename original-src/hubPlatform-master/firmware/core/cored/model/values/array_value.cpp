#include "cored/model/values/array_value.h"
#include "cored/model/exceptions.h"
#include "cored/model/values/value_factory.h"
#include "cored/model/values/value_visitor.h"
#include "cored/model/values/value_cast.h"

#include <cassert>

namespace HubPlatform {
namespace CoreD {
namespace Model {

std::unique_ptr< ArrayValue > ArrayValue::createValue( ValueType arrayType, const Json& values ) {
    if( !values.is_array() && !( values.is_object() && values.empty() ) )
        throw UnexpectedJsonValueFormatError{ values, arrayType };

    auto result = createDefaultValue( std::move( arrayType ) );

    result->m_elements.reserve( values.size() );

    for( auto& value : values ) {
        result->addValue( value );
    }

    return result;
}
std::unique_ptr< ArrayValue > ArrayValue::createDefaultValue( ValueType arrayType ) {
    if( !arrayType.isArray() ) {
        throw UnexpectedValueType( arrayType );
    }

    return std::unique_ptr< ArrayValue >{ new ArrayValue{ std::move( arrayType ) } };
}

Json ArrayValue::toJson() const {
    Json result = Json::array();

    for( const auto& element : m_elements ) {
        result.push_back( element->toJson() );
    }

    return result;
}

void ArrayValue::accept( ValueVisitor& visitor ) {
    visitor.visit( *this );
}

void ArrayValue::accept( ValueConstVisitor& visitor ) const {
    visitor.visit( *this );
}

void ArrayValue::addValue( const Json& value ) {
    m_elements.push_back( ValueFactory::createItemValue( getValuesType(), value ) );
}

std::size_t ArrayValue::getValuesCount() const {
    return m_elements.size();
}

bool ArrayValue::isEqual( const Value& other ) const {
    const auto& otherArray = valueCast< ArrayValue >( other );
    return m_elements.size() == otherArray.m_elements.size() &&
           std::equal( m_elements.begin(),
                       m_elements.end(),
                       otherArray.m_elements.begin(),
                       []( const ValueUPtr& a, const ValueUPtr& b ) { return *a == *b; } );
}

void ArrayValue::set( const Value& other ) {
    const auto& otherArray = valueCast< ArrayValue >( other );
    decltype( m_elements ) elements;

    for( const auto& value : otherArray.m_elements ) {
        auto clonedValue = ValueFactory::createDefaultItemValue( value->getType() );
        *clonedValue = *value;
        elements.push_back( std::move( clonedValue ) );
    }
}

const ValueType& ArrayValue::getValuesType() const {
    assert( getType().getSubType() );
    return *getType().getSubType();
}

void ArrayValue::forEachValue( const ValueHandler& handler ) {
    for( const auto& value : m_elements ) {
        assert( value );
        handler( *value );
    }
}

void ArrayValue::forEachValue( const ConstValueHandler& handler ) const {
    for( const auto& value : m_elements ) {
        assert( value );
        handler( *value );
    }
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
