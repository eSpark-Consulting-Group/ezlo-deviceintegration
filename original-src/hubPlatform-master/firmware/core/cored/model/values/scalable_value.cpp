#include "cored/model/values/scalable_value.h"
#include "cored/model/values/value_visitor.h"
#include "cored/model/values/value_cast.h"
#include "base_utils/float_operations.h"
#include "cored/model/exceptions.h"
#include "log_utils/ezlo_log.h"

#include <cstring>

namespace HubPlatform {
namespace CoreD {
namespace Model {

ScalableValueUPtr ScalableValue::createValue( ValueType type, const Json& json ) {
    if( !json.is_object() ) {
        throw UnexpectedJsonValueFormatError{ json, type };
    }
    const auto valueIt = json.find( "value" );
    const auto scaleIt = json.find( "scale" );
    const auto isRelativeIt = json.find( "relative" );

    if( valueIt == json.end() || scaleIt == json.end() || !valueIt.value().is_number() ||
        !scaleIt.value().is_string() ) {
        throw UnexpectedJsonValueFormatError{ json, type };
    }

    const auto& scales = type.getScales();
    const auto found = std::find( scales.begin(), scales.end(), *scaleIt.value().get< const std::string* >() );
    if( found == scales.end() ) {
        throw ScalesNotFoundError{ type };
    }

    bool isRelative = isRelativeIt != json.end() && isRelativeIt->get< bool >();
    return ScalableValueUPtr{ new ScalableValue( std::move( type ), valueIt->get< float >(), *found, isRelative ) };
}

ScalableValueUPtr ScalableValue::createDefaultValue( ValueType type ) {
    const auto& scales = type.getScales();

    if( scales.begin() == scales.end() ) {
        throw ScalesNotFoundError{ type };
    }

    return ScalableValueUPtr( new ScalableValue{ std::move( type ), 0.f, *scales.begin(), false } );
}

ScalableValue::ScalableValue( ValueType type, float value, ValueScale scale, bool isRelative )
: Value( std::move( type ) )
, m_value( value )
, m_scale( scale )
, m_isRelative( isRelative ) {}

ScalableValue::~ScalableValue() = default;

Json ScalableValue::toJson() const {
    Json result;
    result[ "value" ] = m_value;
    result[ "scale" ] = m_scale;
    if( m_isRelative ) {
        result[ "relative" ] = true;
    }
    return result;
}

void ScalableValue::accept( ValueVisitor& visitor ) {
    visitor.visit( *this );
}

void ScalableValue::accept( ValueConstVisitor& visitor ) const {
    visitor.visit( *this );
}

void ScalableValue::setValue( float value ) {
    m_value = value;
}

float ScalableValue::getValue() const {
    return m_value;
}

float ScalableValue::getConvertedValue( ValueScale otherScale ) const {
    if( strcmp( m_scale, otherScale ) == 0 || strcmp( "", otherScale ) == 0 ) {
        return m_value;
    }

    if( auto converter = getType().getConverter() ) {
        const auto convertedValue = converter( m_value, getScale(), otherScale );
        return isRelative() ? convertedValue - converter( 0.f, getScale(), otherScale ) : convertedValue;
    }

    throw ValueError( "Converter is absent for scalable value" );
}

ValueScale ScalableValue::getScale() const {
    return m_scale;
}

bool ScalableValue::isRelative() const {
    return m_isRelative;
}

bool ScalableValue::isLess( const Value& other ) const {
    const auto& otherScalable = valueCast< ScalableValue >( other );

    if( m_isRelative != otherScalable.m_isRelative ) {
        throw ValueError( "Cannot compare relative and not relative scalable values" );
    }

    return isFloatLess( m_value, otherScalable.getConvertedValue( getScale() ) );
}

bool ScalableValue::isEqual( const Value& other ) const {
    try {
        const auto& otherScalable = valueCast< ScalableValue >( other );
        return m_isRelative == otherScalable.m_isRelative &&
               areFloatsEqual( m_value, otherScalable.getConvertedValue( getScale() ) );
    }
    catch( const ValueError& error ) {
        LOGGER_DEBUG( error.what() );
        return false;
    }
}

void ScalableValue::set( const Value& other ) {
    const auto& otherScalable = valueCast< ScalableValue >( other );
    m_scale = otherScalable.m_scale;
    m_value = otherScalable.m_value;
    m_isRelative = otherScalable.m_isRelative;
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
