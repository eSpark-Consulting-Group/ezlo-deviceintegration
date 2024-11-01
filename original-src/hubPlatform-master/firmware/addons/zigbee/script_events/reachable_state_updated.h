#pragma once

#include "zigbee/script_events/node_event.h"

namespace HubPlatform {
namespace Zigbee {

class ZigbeeNode;

class ReachableStateUpdated : public NodeEvent {
public:
    explicit ReachableStateUpdated( uint16_t nodeId, bool isReachable );

    Json toJson() const final;

private:
    bool m_isReachable;
};

}  // namespace Zigbee
}  // namespace HubPlatform
