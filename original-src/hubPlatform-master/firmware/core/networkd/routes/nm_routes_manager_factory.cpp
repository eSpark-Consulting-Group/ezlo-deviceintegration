#include "networkd/routes/nm_routes_manager_factory.h"

#if defined( __HUB_TYPE_LOCAL__ ) || defined( __HUB_TYPE_VHUB__ )
#include "networkd/routes/nm_dummy_routes_manager.h"
#else
#include "networkd/routes/nm_openwrt_routes_manager.h"
#endif

#include "common_utils/cpp14.h"

#include <memory>

namespace HubPlatform {
namespace Network {
namespace RoutesManagerFactory {

std::unique_ptr< RoutesManager > createRouteManager() {
#if defined( __HUB_TYPE_LOCAL__ ) || defined( __HUB_TYPE_VHUB__ )
    return std::make_unique< DummyRoutesManager >();
#else
    return std::make_unique< OpenwrtRoutesManager >();
#endif
}

}  // namespace RoutesManagerFactory
}  // namespace Network
}  // namespace HubPlatform
