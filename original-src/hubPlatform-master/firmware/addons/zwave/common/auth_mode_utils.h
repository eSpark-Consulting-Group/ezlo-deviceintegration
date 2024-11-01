#pragma once

#include "json_utils/json_fwd.h"

#include <cstdint>

namespace HubPlatform {
namespace Zwave {
namespace AuthModeUtils {

using Modes = std::vector< std::string >;

Modes keyListToModes( std::uint8_t keyList );

std::uint8_t modesToKeyList( const Modes& jaModes );

void checkAuthMode( const char* mode );

}  // namespace AuthModeUtils
}  // namespace Zwave
}  // namespace HubPlatform
