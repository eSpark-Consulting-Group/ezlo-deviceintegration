#include "cored/model/values/geofence_value.h"

#include "cored/model/values/value_visitor.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/model/values/value_cast.h"
#include "cored/model/exceptions.h"

#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

std::unique_ptr< GeofenceValue > GeofenceValue::createValue( const Json& value ) {
    using namespace Resources;

    if( !value.is_object() )
        throw UnexpectedJsonValueFormatError{ value, ValueType::Enum::Geofence, "value is not an object" };

    const std::string FieldModified = GeofenceValueFields::Modified;
    auto modifiedIt = value.find( FieldModified );

    if( modifiedIt == value.cend() )
        throw UnexpectedJsonValueFormatError{
            value, ValueType::Enum::Geofence, "required field \"" + FieldModified + "\" not found"
        };

    if( !modifiedIt->is_number_integer() )
        throw UnexpectedJsonValueFormatError{
            value, ValueType::Enum::Geofence, "required field \"" + FieldModified + "\" is not integer"
        };

    int modifiedTime = modifiedIt->get< int >();
    if( modifiedTime < 0 )
        throw UnexpectedJsonValueFormatError{
            value, ValueType::Enum::Geofence, "invalid field \"" + FieldModified + "\" value should not be negative"
        };

    return std::unique_ptr< GeofenceValue >{ new GeofenceValue{ modifiedTime } };
}

std::unique_ptr< GeofenceValue > GeofenceValue::createDefaultValue() {
    constexpr int modifiedTime = 0;
    return std::unique_ptr< GeofenceValue >{ new GeofenceValue{ modifiedTime } };
}

Json GeofenceValue::toJson() const {
    namespace Fields = Resources::GeofenceValueFields;
    return { { Fields::Modified, m_modifiedTime } };
}

void GeofenceValue::accept( ValueVisitor& visitor ) {
    visitor.visit( *this );
}

void GeofenceValue::accept( ValueConstVisitor& visitor ) const {
    visitor.visit( *this );
}

bool GeofenceValue::isEqual( const Value& other ) const {
    const auto& object = valueCast< GeofenceValue >( other );
    return m_modifiedTime == object.m_modifiedTime;
}

void GeofenceValue::set( const Value& other ) {
    const auto& object = valueCast< GeofenceValue >( other );
    m_modifiedTime = object.m_modifiedTime;
}

GeofenceValue::GeofenceValue( int modifiedTime )
: Value( ValueType::Enum::Geofence )
, m_modifiedTime( modifiedTime ) {}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
