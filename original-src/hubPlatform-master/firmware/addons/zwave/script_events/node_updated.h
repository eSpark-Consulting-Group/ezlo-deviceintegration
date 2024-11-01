#pragma once

#include "zwave/script_events/node_event.h"
#include "zwave/resources/zwave_resources.h"

namespace HubPlatform {
namespace Zwave {

class NodeUpdated : public NodeEvent {
public:
    explicit NodeUpdated( uint8_t nodeId )
    : NodeEvent( Resources::ScriptEvent::Name::NodeUpdated, nodeId ) {}
};

}  // namespace Zwave
}  // namespace HubPlatform
