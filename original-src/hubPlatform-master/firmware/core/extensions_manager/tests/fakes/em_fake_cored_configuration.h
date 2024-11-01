#pragma once

#include "cored/model/configuration/configuration.h"
#include "cored/model/values/value_type.h"

namespace HubPlatform {
namespace CoreDRemoteInterface {
namespace UT {

class FakeConfiguration final : public CoreD::Model::Configuration {
public:
    ~FakeConfiguration() override = default;

    std::string getScaleForValueType( const CoreD::Model::ValueType& valueType ) const override {
        return valueType == CoreD::Model::ValueType::Enum::Temperature ? CoreD::Model::ValueScales::Fahrenheit : "";
    }
};

}  // namespace UT
}  // namespace CoreDRemoteInterface
}  // namespace HubPlatform
