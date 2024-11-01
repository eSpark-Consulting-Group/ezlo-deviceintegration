#pragma once

#include <string>

namespace HubPlatform {
namespace CoreD {
namespace Model {

enum class GatewayStatus { NotStarted, Starting, Ready };

std::string toString( GatewayStatus status );

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
