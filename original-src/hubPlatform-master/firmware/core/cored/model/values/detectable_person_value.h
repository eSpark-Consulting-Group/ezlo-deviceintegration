#pragma once
#include "cored/model/values/value.h"
#include "cored/model/values/detectable_object_base.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

class DetectablePersonValue : public DetectableObjectBase {
public:
    static std::unique_ptr< DetectablePersonValue > createValue( const Json& value );
    static std::unique_ptr< DetectablePersonValue > createDefaultValue();

    void accept( ValueVisitor& visitor ) override;
    void accept( ValueConstVisitor& visitor ) const override;

private:
    DetectablePersonValue( const Json& value );
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
