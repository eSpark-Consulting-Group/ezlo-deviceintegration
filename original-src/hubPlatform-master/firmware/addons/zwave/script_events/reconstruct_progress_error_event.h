#pragma once

#include "zwave/script_events/node_event.h"
#include "zwave/resources/zwave_resources.h"

namespace HubPlatform {
namespace Zwave {

class ReconstructProgressErrorEvent : public NodeEvent {
public:
    explicit ReconstructProgressErrorEvent( uint8_t nodeId )
    : NodeEvent( Resources::ScriptEvent::Name::ReconstructProgressError, nodeId ) {}
};

}  // namespace Zwave
}  // namespace HubPlatform
