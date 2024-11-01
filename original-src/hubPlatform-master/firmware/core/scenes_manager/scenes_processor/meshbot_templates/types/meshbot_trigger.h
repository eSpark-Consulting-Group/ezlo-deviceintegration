#pragma once

#include "json_utils/json_fwd.h"

#include <string>

namespace HubPlatform {
namespace ScenesManager {
namespace MeshbotTemplates {
namespace Types {

struct MeshbotTrigger {
    std::string deviceId;
    Json device;

    std::string itemId;
    Json item;
};

}  // namespace Types
}  // namespace MeshbotTemplates
}  // namespace ScenesManager
}  // namespace HubPlatform
