#pragma once

#include "networkd/routes/nm_routes_manager.h"

#include <memory>

namespace HubPlatform {
namespace Network {
namespace RoutesManagerFactory {

std::unique_ptr< RoutesManager > createRouteManager();

}  // namespace RoutesManagerFactory
}  // namespace Network
}  // namespace HubPlatform
