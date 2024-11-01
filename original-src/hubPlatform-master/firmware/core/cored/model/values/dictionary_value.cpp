#include "cored/model/values/dictionary_value.h"

#include "cored/model/values/value_visitor.h"
#include "cored/model/values/value_cast.h"
#include "cored/model/exceptions.h"
#include "cored/model/values/value_factory.h"

#include "common_utils/cpp14.h"
#include "json_utils/json_wrapper.h"

#include <algorithm>
#include <cassert>

namespace HubPlatform {
namespace CoreD {
namespace Model {

std::unique_ptr< DictionaryValue > DictionaryValue::createValue( ValueType dictionaryType, const Json& values ) {
    if( !values.is_object() )
        throw UnexpectedJsonValueFormatError{ values, dictionaryType };

    auto dictionary = createDefaultValue( std::move( dictionaryType ) );

    for( auto it = values.begin(); it != values.end(); ++it ) {
        dictionary->addValue( it.key(), it.value() );
    }

    return dictionary;
}

std::unique_ptr< DictionaryValue > DictionaryValue::createDefaultValue( ValueType dictionaryType ) {
    if( !dictionaryType.isDictionary() ) {
        throw UnexpectedValueType( dictionaryType );
    }

    return std::unique_ptr< DictionaryValue >{ new DictionaryValue{ std::move( dictionaryType ) } };
}

DictionaryValue::~DictionaryValue() = default;

Json DictionaryValue::toJson() const {
    auto result = JSON_EMPTY_OBJ;
    for( const auto& it : m_values ) {
        result[ it.first ] = it.second->toJson();
    }
    return result;
}

void DictionaryValue::accept( ValueVisitor& visitor ) {
    visitor.visit( *this );
}

void DictionaryValue::accept( ValueConstVisitor& visitor ) const {
    visitor.visit( *this );
}

const ValueType& DictionaryValue::getValuesType() const {
    return *getType().getSubType();
}

std::size_t DictionaryValue::getValuesCount() const {
    return m_values.size();
}

bool DictionaryValue::isEqual( const Value& other ) const {
    const auto& otherDictionary = valueCast< DictionaryValue >( other );

    if( getValuesCount() != otherDictionary.getValuesCount() ) {
        return false;
    }

    return std::equal( m_values.cbegin(),
                       m_values.cend(),
                       otherDictionary.m_values.cbegin(),
                       []( const decltype( m_values )::value_type& lhs, const decltype( m_values )::value_type& rhs ) {
                           return lhs.first == rhs.first && *lhs.second == *rhs.second;
                       } );
}

void DictionaryValue::set( const Value& value ) {
    const auto& otherDictionary = valueCast< DictionaryValue >( value );
    decltype( m_values ) values;

    for( const auto& valuePair : otherDictionary.m_values ) {
        auto value = ValueFactory::createDefaultItemValue( valuePair.second->getType() );
        *value = *valuePair.second;
        values.emplace( valuePair.first, std::move( value ) );
    }
    m_values = std::move( values );
}

void DictionaryValue::addValue( const ValueId& key, const Json& value ) {
    m_values.insert( std::make_pair( key, ValueFactory::createItemValue( getValuesType(), value ) ) );
}

void DictionaryValue::setValue( const ValueId& key, ValueUPtr value ) {
    if( !value ) {
        removeValue( key );
    }
    else {
        if( value->getType() != getValuesType() ) {
            throw UnexpectedValueType( value->getType() );
        }

        m_values[ key ] = std::move( value );
    }
}

const Value* DictionaryValue::getValue( const ValueId& key ) const {
    auto it = m_values.find( key );
    return it != m_values.end() ? it->second.get() : nullptr;
}

void DictionaryValue::removeValue( const ValueId& key ) {
    m_values.erase( key );
}

void DictionaryValue::forEachValue( ConstValueHandler handler ) const {
    for( const auto& idToValue : m_values ) {
        handler( idToValue.first, *idToValue.second );
    }
}

void DictionaryValue::forEachValue( ValueHandler handler ) {
    for( auto& idToValue : m_values ) {
        handler( idToValue.first, *idToValue.second );
    }
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
