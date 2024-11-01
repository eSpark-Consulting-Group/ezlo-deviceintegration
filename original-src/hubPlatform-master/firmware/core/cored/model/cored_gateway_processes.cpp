#include "cored/model/cored_gateway_processes.h"
#include "base_utils/enums/enum_from_string.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

std::string toString( GatewayProcess process ) {
    switch( process ) {
    case GatewayProcess::AddingDevice:
        return "adding_device";
    case GatewayProcess::RemovingDevice:
        return "removing_device";
    default:
        return "";
    }
}

std::string toString( GatewayProcessResult processResult ) {
    switch( processResult ) {
    case GatewayProcessResult::Success:
        return "success";
    case GatewayProcessResult::Fail:
        return "fail";
    case GatewayProcessResult::Unspecified:
        return "unspecified";
    default:
        return "";
    }
}

GatewayProcess gatewayProcessFromString( const std::string& value ) {
    return enumFromString< GatewayProcess >( value );
}

GatewayProcessResult gatewayProcessResultFromString( const std::string& value ) {
    return enumFromString< GatewayProcessResult >( value );
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
