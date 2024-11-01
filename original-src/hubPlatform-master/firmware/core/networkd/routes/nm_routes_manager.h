#pragma once

#include "json_utils/json_fwd.h"

#include <string>

namespace HubPlatform {
namespace Network {

class RoutesManager {
public:
    virtual ~RoutesManager() = default;

    virtual std::string getDefaultRouteDevice() = 0;
    virtual std::string checkAndFixRoutes( const std::vector< Json >& interfaces ) = 0;
};

}  // namespace Network
}  // namespace HubPlatform
