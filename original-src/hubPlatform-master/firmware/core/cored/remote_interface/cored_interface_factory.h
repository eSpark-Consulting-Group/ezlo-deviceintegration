#pragma once

#include "cored/remote_interface/cored_interface.h"

namespace HubPlatform {

namespace Services {
class Service;
}  // namespace Services

namespace CoreDRemoteInterface {
CoreDInterfaceUPtr makeInstance( Services::Service& hostService );
}
}  // namespace HubPlatform