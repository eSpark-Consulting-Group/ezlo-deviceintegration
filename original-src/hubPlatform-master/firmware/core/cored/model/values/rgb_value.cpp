#include "cored/model/values/rgb_value.h"

#include "cored/model/values/value_visitor.h"
#include "cored/model/values/value_type.h"
#include "cored/model/values/value_cast.h"
#include "cored/model/exceptions.h"
#include "cored/model/resources/cored_resources.h"

#include "json_utils/json_wrapper.h"
#include "json_utils/json_utils.h"
#include "common_utils/cpp14.h"
#include "base_utils/comparators.h"
#include "base_utils/float_operations.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

std::unique_ptr< RgbValue > RgbValue::createValue( const JsonObject& value ) {
    if( !value.is_object() )
        throw UnexpectedJsonValueFormatError{ value, ValueType::Enum::Rgb };

    std::unique_ptr< RgbValue > rgbValue{ new RgbValue{} };

    for( auto it = value.begin(); it != value.end(); ++it ) {
        const std::string colorStr{ it.key() };
        const Color color{ colorFromString( colorStr ) };
        if( color == Color::Unknown )
            throw ValueError{ "Unsupported color id: " + colorStr };

        int val{ 0 };
        const auto& jValue = it.value();
        if( jValue.is_number() )
            val = jValue.get< int >();
        else
            throw ValueError{ "Color value type must be number" };

        if( val < Resources::Constants::LowerLimitColorValue )
            val = Resources::Constants::LowerLimitColorValue;
        else if( val > Resources::Constants::UpperLimitColorValue )
            val = Resources::Constants::UpperLimitColorValue;

        rgbValue->setComponentValue( color, static_cast< uint8_t >( val ) );
    }

    return rgbValue;
}

std::unique_ptr< RgbValue > RgbValue::createDefaultValue() {
    return std::unique_ptr< RgbValue >{ new RgbValue{} };
}

RgbValue::RgbValue()
: Value( ValueType::Enum::Rgb ) {}

RgbValue::~RgbValue() = default;

Json RgbValue::toJson() const {
    JsonObject json;
    forEachComponent( [&json]( Color id, uint8_t value ) {
        json[ colorToString( id ) ] = value;
        return true;
    } );

    return json.empty() ? JSON_EMPTY_OBJ : json;
}

bool RgbValue::isEqual( const Value& other ) const {
    const auto& otherRgbValue = valueCast< RgbValue >( other );
    return m_components == otherRgbValue.m_components;
}

void RgbValue::set( const Value& other ) {
    const auto& otherRgbValue = valueCast< RgbValue >( other );
    m_components = otherRgbValue.m_components;
}

void RgbValue::accept( ValueVisitor& visitor ) {
    visitor.visit( *this );
}

void RgbValue::accept( ValueConstVisitor& visitor ) const {
    visitor.visit( *this );
}

bool RgbValue::forEachComponent( std::function< bool( Color, uint8_t ) > _callBack ) const {
    for( auto&& component : m_components ) {
        if( !_callBack( component.first, component.second ) )
            return false;
    }
    return true;
}

bool RgbValue::update( Color componentId, uint8_t value ) {
    const auto it = m_components.find( componentId );
    if( it == m_components.end() || it->second != value ) {
        setComponentValue( componentId, value );
        return true;
    }

    return false;
}

uint8_t RgbValue::getColor( Color color ) const {
    auto it = m_components.find( color );
    if( it != m_components.end() )
        return it->second;

    return Resources::Constants::LowerLimitColorValue;
}

void RgbValue::setComponentValue( Color componentId, uint8_t value ) {
    using namespace Resources;
    m_components[ componentId ] = value;
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
