#pragma once

#include "watcher/types/process_pid.h"

#include <string>

namespace HubPlatform {
namespace Watcher {
namespace Types {

enum class ZWaveNodeStatus { ZWAVE_NO_OPERATION, ZWAVE_NODE_EXPIRED, ZWAVE_NODE_NOT_EXPIRED, ZWAVE_NODE_NIL };

}  // namespace Types
}  // namespace Watcher
}  // namespace HubPlatform
