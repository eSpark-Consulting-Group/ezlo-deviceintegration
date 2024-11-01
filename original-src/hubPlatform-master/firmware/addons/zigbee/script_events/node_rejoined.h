#pragma once

#include "zigbee/script_events/node_event.h"

namespace HubPlatform {
namespace Zigbee {

class ZigbeeNode;

class NodeRejoined : public NodeEvent {
public:
    explicit NodeRejoined( uint16_t nodeId );
};

}  // namespace Zigbee
}  // namespace HubPlatform
