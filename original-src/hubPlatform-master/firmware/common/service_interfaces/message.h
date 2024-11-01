#pragma once

#include "json_utils/json_fwd.h"

#include <string>

namespace HubPlatform {
namespace Services {
struct Message {
    std::string group;
    int msgClass;
    int msgSubClass;
    JsonObject data;
};
}  // namespace Services
}  // namespace HubPlatform
