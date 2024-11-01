#pragma once

#include <string>

namespace HubPlatform {
namespace CoreD {
namespace Model {

enum class ManualDeviceAddingCapability { No, Wizard };

std::string toString( ManualDeviceAddingCapability status );

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
