#pragma once

#include "vidoo4hub/vidoo_client/vidoo_client_interface.h"
#include "service_interfaces/service.h"

#include <memory>

namespace HubPlatform {
namespace Vidoo4Hub {

std::unique_ptr< IVidooClient > createVidooClient( Services::Service& service, IVidooClient::MessageCallback callback );

}  // namespace Vidoo4Hub
}  // namespace HubPlatform
