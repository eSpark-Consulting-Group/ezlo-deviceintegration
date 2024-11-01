#include "cored/model/values/led_effect_value.h"

#include "cored/model/exceptions.h"
#include "cored/model/values/value_visitor.h"
#include "cored/model/values/value_type.h"
#include "cored/model/values/value_cast.h"

#include "cored/model/resources/cored_resources.h"

#include "json_utils/json_wrapper.h"
#include "json_utils/json_utils.h"
#include "json_utils/json_custom_getters.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

std::unique_ptr< LedEffectValue > LedEffectValue::createValue( const Json& value ) {
    using namespace Resources::LedEffectValueFields;
    using namespace JsonCustomGetters;

    auto result = createDefaultValue();

    const auto getter = makeStructGetter(
        makeMemberMapping( &LedEffectValue::m_effectName, EffectNameFieldName ),
        makeMemberMapping( &LedEffectValue::m_effectDescription, EffectDescriptionFieldName, OptionalMemberPolicy() ),
        makeMemberMapping(
            &LedEffectValue::m_effectHeadpieceUrl, EffectHeadpieceUrlFieldName, OptionalMemberPolicy() ) );

    if( !getter( value, *result ) ) {
        throw UnexpectedJsonValueFormatError{ value, ValueType::Enum::LedEffect };
    }

    return result;
}

std::unique_ptr< LedEffectValue > LedEffectValue::createDefaultValue() {
    return std::unique_ptr< LedEffectValue >{ new LedEffectValue{ {}, {}, {} } };
}

LedEffectValue::LedEffectValue( std::string effectName, std::string effectDescription, std::string effectHeadpieceUrl )
: Value( ValueType::Enum::LedEffect )
, m_effectName( std::move( effectName ) )
, m_effectDescription( std::move( effectDescription ) )
, m_effectHeadpieceUrl( std::move( effectHeadpieceUrl ) ) {}

LedEffectValue::~LedEffectValue() = default;

Json LedEffectValue::toJson() const {
    using namespace Resources::LedEffectValueFields;

    auto result = Json::object();

    result[ EffectNameFieldName ] = m_effectName;

    if( !m_effectDescription.empty() ) {
        result[ EffectDescriptionFieldName ] = m_effectDescription;
    }

    if( !m_effectHeadpieceUrl.empty() ) {
        result[ EffectHeadpieceUrlFieldName ] = m_effectHeadpieceUrl;
    }

    return result;
}

bool LedEffectValue::isEqual( const Value& other ) const {
    const auto& otherEffectValue = valueCast< LedEffectValue >( other );
    return m_effectName == otherEffectValue.m_effectName &&
           m_effectDescription == otherEffectValue.m_effectDescription &&
           m_effectHeadpieceUrl == otherEffectValue.m_effectHeadpieceUrl;
}

void LedEffectValue::set( const Value& other ) {
    const auto& otherUserCodeValue = valueCast< LedEffectValue >( other );
    m_effectName = otherUserCodeValue.m_effectName;
    m_effectDescription = otherUserCodeValue.m_effectDescription;
    m_effectHeadpieceUrl = otherUserCodeValue.m_effectHeadpieceUrl;
}

void LedEffectValue::accept( ValueVisitor& visitor ) {
    visitor.visit( *this );
}

void LedEffectValue::accept( ValueConstVisitor& visitor ) const {
    visitor.visit( *this );
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
