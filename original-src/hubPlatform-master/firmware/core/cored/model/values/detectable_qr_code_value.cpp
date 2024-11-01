#include "cored/model/values/detectable_qr_code_value.h"
#include "cored/model/values/value_visitor.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/model/exceptions.h"

#include "json_utils/json_custom_getters.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

DetectableQRCodeValue::DetectableQRCodeValue( const Json& value )
: DetectableObjectBase( value, ValueType::Enum::DetectableQRCode ) {}

std::unique_ptr< DetectableQRCodeValue > DetectableQRCodeValue::createValue( const Json& value ) {
    DetectableObjectBase::validateValue( value, ValueType::Enum::DetectableQRCode );
    return std::unique_ptr< DetectableQRCodeValue >{ new DetectableQRCodeValue{ value } };
}

std::unique_ptr< DetectableQRCodeValue > DetectableQRCodeValue::createDefaultValue() {
    return std::unique_ptr< DetectableQRCodeValue >{ new DetectableQRCodeValue{
        DetectableObjectBase::getDefaultValue( ValueType::Enum::DetectableQRCode ) } };
}

void DetectableQRCodeValue::accept( ValueVisitor& visitor ) {
    visitor.visit( *this );
}

void DetectableQRCodeValue::accept( ValueConstVisitor& visitor ) const {
    visitor.visit( *this );
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
