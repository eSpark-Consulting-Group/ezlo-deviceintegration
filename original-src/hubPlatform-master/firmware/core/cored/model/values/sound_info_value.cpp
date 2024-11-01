#include "cored/model/values/sound_info_value.h"

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

template < typename T, JsonObject::value_t JsonT >
static T getValue( const JsonObject& obj, const std::string& key ) {
    auto it = obj.find( key );
    if( it != obj.end() && it->type() == JsonT ) {
        return it->get< T >();
    }
    else {
        throw UnexpectedJsonValueFormatError{ obj, ValueType::Enum::SoundInfo };
    }
}

std::unique_ptr< SoundInfoValue > SoundInfoValue::createValue( const JsonObject& value ) {
    using namespace Resources::SoundInfoValueFields;

    if( !value.is_object() )
        throw UnexpectedJsonValueFormatError{ value, ValueType::Enum::SoundInfo };

    auto name = getValue< std::string, JsonObject::value_t::string >( value, Name );
    auto duration = getValue< int, JsonObject::value_t::number_integer >( value, Duration );

    return std::unique_ptr< SoundInfoValue >{ new SoundInfoValue( std::move( name ), duration ) };
}

std::unique_ptr< SoundInfoValue > SoundInfoValue::createDefaultValue() {
    return std::unique_ptr< SoundInfoValue >{ new SoundInfoValue( "", 0 ) };
}

SoundInfoValue::SoundInfoValue( std::string&& name, int duration )
: Value( ValueType::Enum::SoundInfo )
, m_name( std::move( name ) )
, m_duration( duration ) {}

SoundInfoValue::~SoundInfoValue() = default;

Json SoundInfoValue::toJson() const {
    using namespace Resources::SoundInfoValueFields;

    JsonObject json;

    json[ Name ] = m_name;
    json[ Duration ] = m_duration;

    return json;
}

bool SoundInfoValue::isEqual( const Value& other ) const {
    const auto& otherSoundInfoValue = valueCast< SoundInfoValue >( other );

    return m_name == otherSoundInfoValue.m_name && m_duration == otherSoundInfoValue.m_duration;
}

void SoundInfoValue::set( const Value& other ) {
    const auto& otherSoundInfoValue = valueCast< SoundInfoValue >( other );

    m_name = otherSoundInfoValue.m_name;
    m_duration = otherSoundInfoValue.m_duration;
}

void SoundInfoValue::accept( ValueVisitor& visitor ) {
    visitor.visit( *this );
}

void SoundInfoValue::accept( ValueConstVisitor& visitor ) const {
    visitor.visit( *this );
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
