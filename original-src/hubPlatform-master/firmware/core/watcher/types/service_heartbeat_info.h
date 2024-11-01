#pragma once

#include "watcher/types/process_pid.h"

#include <string>
#include <chrono>

namespace HubPlatform {
namespace Watcher {
namespace Types {

struct ServiceHeartbeatInfo {
    Pid pid{};
    std::string serviceName;
    std::chrono::steady_clock::time_point lastUpdateTime;
};

}  // namespace Types
}  // namespace Watcher
}  // namespace HubPlatform
