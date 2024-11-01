#pragma once

#include "json_utils/json_fwd.h"

namespace HubPlatform {
namespace UpdateManager {

enum class HubResetType : uint8_t { Soft, Factory };

HubResetType hubResetTypeFromJson( const JsonObject& json );
JsonObject hubResetTypeToJson( HubResetType resetType );

}  // namespace UpdateManager
}  // namespace HubPlatform
