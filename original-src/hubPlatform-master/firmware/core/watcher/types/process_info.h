#pragma once

#include "watcher/types/process_pid.h"

#include <string>

namespace HubPlatform {
namespace Watcher {
namespace Types {

struct ProcessInfo {
    Pid pid{};
    std::string state;
    std::string cmd;
};

}  // namespace Types
}  // namespace Watcher
}  // namespace HubPlatform
