
#pragma once
#include "cored/model/values/value.h"
#include "cored/model/values/detectable_object_base.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

class DetectableQRCodeValue : public DetectableObjectBase {
public:
    static std::unique_ptr< DetectableQRCodeValue > createValue( const Json& value );
    static std::unique_ptr< DetectableQRCodeValue > createDefaultValue();

    void accept( ValueVisitor& visitor ) override;
    void accept( ValueConstVisitor& visitor ) const override;

private:
    DetectableQRCodeValue( const Json& value );
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
