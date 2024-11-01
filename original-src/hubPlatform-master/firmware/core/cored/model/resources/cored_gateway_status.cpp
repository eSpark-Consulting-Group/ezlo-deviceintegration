#include "cored/model/resources/cored_gateway_status.h"
#include "cored/model/resources/cored_resources.h"

#include <cassert>

namespace HubPlatform {
namespace CoreD {
namespace Model {

std::string toString( GatewayStatus status ) {
    switch( status ) {
    case GatewayStatus::NotStarted:
        return Resources::GatewayStatus::NotStarted;

    case GatewayStatus::Starting:
        return Resources::GatewayStatus::Starting;

    case GatewayStatus::Ready:
        return Resources::GatewayStatus::Ready;

    default:
        assert( false && "Unsupported gateway status" );
        return "";
    }
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
