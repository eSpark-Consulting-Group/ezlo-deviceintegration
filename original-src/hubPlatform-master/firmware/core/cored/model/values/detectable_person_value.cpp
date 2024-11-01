#include "cored/model/values/detectable_person_value.h"
#include "cored/model/values/value_visitor.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/model/exceptions.h"

#include "json_utils/json_custom_getters.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

DetectablePersonValue::DetectablePersonValue( const Json& value )
: DetectableObjectBase( value, ValueType::Enum::DetectablePerson ) {}

std::unique_ptr< DetectablePersonValue > DetectablePersonValue::createValue( const Json& value ) {
    DetectableObjectBase::validateValue( value, ValueType::Enum::DetectablePerson );
    return std::unique_ptr< DetectablePersonValue >{ new DetectablePersonValue{ value } };
}

std::unique_ptr< DetectablePersonValue > DetectablePersonValue::createDefaultValue() {
    return std::unique_ptr< DetectablePersonValue >{ new DetectablePersonValue{
        DetectableObjectBase::getDefaultValue( ValueType::Enum::DetectablePerson ) } };
}

void DetectablePersonValue::accept( ValueVisitor& visitor ) {
    visitor.visit( *this );
}

void DetectablePersonValue::accept( ValueConstVisitor& visitor ) const {
    visitor.visit( *this );
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
