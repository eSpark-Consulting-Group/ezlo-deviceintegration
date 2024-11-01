#pragma once

#include "json_utils/json_fwd.h"

namespace HubPlatform {

namespace Services {
class Service;
}

namespace CoreD {
namespace Protect {
class ProtectRemoteInterface {
public:
    virtual ~ProtectRemoteInterface() = default;
    virtual JsonObject list( const JsonObject& params ) = 0;
};
}  // namespace Protect
}  // namespace CoreD
}  // namespace HubPlatform