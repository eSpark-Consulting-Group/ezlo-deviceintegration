#pragma once

#include "zwave/script_events/node_event.h"
#include "zwave/resources/zwave_resources.h"

namespace HubPlatform {
namespace Zwave {

class NodeAdded : public NodeEvent {
public:
    explicit NodeAdded( uint8_t nodeId )
    : NodeEvent( Resources::ScriptEvent::Name::NodeAdded, nodeId ) {}
};

}  // namespace Zwave
}  // namespace HubPlatform
