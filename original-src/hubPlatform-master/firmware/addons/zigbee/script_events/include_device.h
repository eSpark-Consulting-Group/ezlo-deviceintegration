#pragma once

#include "zigbee/script_events/status_event.h"

namespace HubPlatform {
namespace Zigbee {

class IncludeDevice : public StatusEvent {
public:
    explicit IncludeDevice( ModeStatus status );
};

}  // namespace Zigbee
}  // namespace HubPlatform
