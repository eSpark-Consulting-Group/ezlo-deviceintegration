#pragma once

#include "zigbee/script_events/status_event.h"

namespace HubPlatform {
namespace Zigbee {

class ModuleReset : public StatusEvent {
public:
    explicit ModuleReset( ModeStatus status );
};

}  // namespace Zigbee
}  // namespace HubPlatform
