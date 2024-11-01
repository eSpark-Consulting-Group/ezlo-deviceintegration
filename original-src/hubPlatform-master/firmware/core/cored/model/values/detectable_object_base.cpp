#include "cored/model/values/detectable_object_base.h"
#include "cored/model/values/value_visitor.h"
#include "cored/model/values/value_cast.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/model/exceptions.h"

#include "json_utils/json_custom_getters.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

DetectableObjectBase::~DetectableObjectBase() = default;

Json DetectableObjectBase::toJson() const {
    return m_value;
}

bool DetectableObjectBase::isEqual( const Value& other ) const {
    const auto& object = valueCast< DetectableObjectBase >( other );
    return m_value.dump() == object.m_value.dump();
}

void DetectableObjectBase::set( const Value& other ) {
    const auto& object = valueCast< DetectableObjectBase >( other );
    m_value = object.m_value;
}

void DetectableObjectBase::validateValue( const Json& value, ValueType::Enum type ) {
    using namespace Resources;

    const bool hasNameField = true;  // NOTE: this field is present for all detectable object types
    const bool hasValueField =
        ( type != ValueType::Enum::DetectableObject && type != ValueType::Enum::DetectablePerson );

    auto throwOnError = [&value, &type]( bool status ) {
        if( !status )
            throw UnexpectedJsonValueFormatError{ value, type };
    };

    auto valueNotEmpty = []( const Json& json, const std::string& key ) {
        auto it = json.find( key );
        if( it != json.cend() && it.value().is_string() )
            return !it.value().empty();
        return false;
    };

    throwOnError( value.is_object() );
    throwOnError( !value.empty() );
    throwOnError( value.find( DetectableObjectBaseFields::Name ) != value.cend() );

    if( hasValueField )
        throwOnError( value.find( DetectableObjectBaseFields::Value ) != value.cend() );

    auto lastHotzoneIt = value.find( DetectableObjectBaseFields::LastHotzone );
    const bool hasHotzoneField = ( lastHotzoneIt != value.cend() );
    const int fieldsNum = hasNameField + hasValueField + hasHotzoneField;
    throwOnError( value.size() == fieldsNum );

    if( hasHotzoneField ) {
        auto& hotzone = *lastHotzoneIt;
        throwOnError( hotzone.size() == 3 );
        throwOnError( valueNotEmpty( hotzone, DetectableObjectBaseFields::Date ) );
        throwOnError( valueNotEmpty( hotzone, DetectableObjectBaseFields::ItemId ) );
        throwOnError( hotzone.find( DetectableObjectBaseFields::Id ) != hotzone.cend() );
    }
}

Json DetectableObjectBase::getDefaultValue( ValueType::Enum type ) {
    namespace Fields = Resources::DetectableObjectBaseFields;
    if( type == ValueType::Enum::DetectableObject || type == ValueType::Enum::DetectablePerson )
        return JSON( Fields::Name << "" );
    return JSON( Fields::Name << "" << Fields::Value << "" );
}

DetectableObjectBase::DetectableObjectBase( Json value, ValueType::Enum type )
: Value( type )
, m_value( std::move( value ) ) {}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
