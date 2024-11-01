#include "cored/model/values/detectable_object_value.h"
#include "cored/model/values/value_visitor.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/model/exceptions.h"

#include "json_utils/json_custom_getters.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

DetectableObjectValue::DetectableObjectValue( const Json& value )
: DetectableObjectBase( value, ValueType::Enum::DetectableObject ) {}

std::unique_ptr< DetectableObjectValue > DetectableObjectValue::createValue( const Json& value ) {
    DetectableObjectBase::validateValue( value, ValueType::Enum::DetectableObject );
    return std::unique_ptr< DetectableObjectValue >{ new DetectableObjectValue{ value } };
}

std::unique_ptr< DetectableObjectValue > DetectableObjectValue::createDefaultValue() {
    return std::unique_ptr< DetectableObjectValue >{ new DetectableObjectValue{
        DetectableObjectBase::getDefaultValue( ValueType::Enum::DetectableObject ) } };
}

void DetectableObjectValue::accept( ValueVisitor& visitor ) {
    visitor.visit( *this );
}

void DetectableObjectValue::accept( ValueConstVisitor& visitor ) const {
    visitor.visit( *this );
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
