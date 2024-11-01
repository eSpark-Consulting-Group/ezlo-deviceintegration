#pragma once

#include "json_utils/json_fwd.h"
#include "service_interfaces/service.h"

namespace HubPlatform {
namespace Uid {
namespace Common {

JsonObject getHubInfo( Services::Service& service );

}  // namespace Common
}  // namespace Uid
}  // namespace HubPlatform
