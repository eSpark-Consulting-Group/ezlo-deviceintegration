#pragma once

#include "zwave/script_events/node_event.h"

namespace HubPlatform {
namespace Zwave {

class ReachableStateUpdated : public NodeEvent {
public:
    ReachableStateUpdated( uint8_t nodeId, bool value );

    Json toJson() const final;

private:
    bool m_value;
};

}  // namespace Zwave
}  // namespace HubPlatform
