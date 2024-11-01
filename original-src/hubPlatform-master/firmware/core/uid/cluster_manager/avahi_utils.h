#pragma once

#include "json_utils/json_fwd.h"

namespace HubPlatform {
namespace Uid {
namespace AvahiUtils {

// NOTE: expecting output of `avahi-browse -pktr` or equivalent
std::vector< Json > parseAvahiBrowseOutput( const std::string& output );

}  // namespace AvahiUtils
}  // namespace Uid
}  // namespace HubPlatform
