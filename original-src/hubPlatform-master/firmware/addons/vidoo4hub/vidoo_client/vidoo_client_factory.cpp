#include "vidoo4hub/vidoo_client/vidoo_client_factory.h"
#include "vidoo4hub/vidoo_client/vidoo_mqtt_client.h"
#include "vidoo4hub/vidoo_client/vidoo_bus_client.h"

#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace Vidoo4Hub {

std::unique_ptr< IVidooClient > createVidooClient( Services::Service& service,
                                                   IVidooClient::MessageCallback callback ) {
#if __HUB_TYPE_VHUB__
    return std::make_unique< VidooMQTTClient >( service, std::move( callback ) );
#else
    return std::make_unique< VidooBusClient >( service, std::move( callback ) );
#endif
}

}  // namespace Vidoo4Hub
}  // namespace HubPlatform
