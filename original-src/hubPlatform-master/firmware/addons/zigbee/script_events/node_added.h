#pragma once

#include "zigbee/script_events/node_event.h"

namespace HubPlatform {
namespace Zigbee {

class ZigbeeNode;

class NodeAdded : public NodeEvent {
public:
    explicit NodeAdded( uint16_t nodeId );
};

}  // namespace Zigbee
}  // namespace HubPlatform
