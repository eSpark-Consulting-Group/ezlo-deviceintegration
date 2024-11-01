#include "cored/remote_interface/cored_interface_factory.h"
#include "cored/remote_interface/cored_remote_interface.h"

namespace HubPlatform {
namespace CoreDRemoteInterface {

CoreDInterfaceUPtr makeInstance( Services::Service& hostService ) {
    return std::make_unique< CoreDRemoteInterface >( hostService );
}

}  // namespace CoreDRemoteInterface
}  // namespace HubPlatform