#pragma once

#include <string>

namespace HubPlatform {
namespace CoreD {
namespace Model {

enum class GatewayProcess { Unknown, AddingDevice, RemovingDevice, End, Begin = Unknown, Default = Unknown };

enum class GatewayProcessResult { Unspecified, Success, Fail, End, Begin = Unspecified, Default = Unspecified };

std::string toString( GatewayProcess process );
std::string toString( GatewayProcessResult processResult );

GatewayProcess gatewayProcessFromString( const std::string& value );
GatewayProcessResult gatewayProcessResultFromString( const std::string& value );

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
