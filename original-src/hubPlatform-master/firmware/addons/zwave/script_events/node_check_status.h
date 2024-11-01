#pragma once

#include "zwave/resources/zwave_resources.h"

#include "zwave/script_events/node_event.h"

#include "znet/znet_command.hpp"

namespace HubPlatform {
namespace Zwave {

class NodeCheckStatus : public NodeEvent {
public:
    NodeCheckStatus( uint8_t nodeId, znet::ModeStatus status );

    Json toJson() const override;

private:
    const znet::ModeStatus m_status;
};

}  // namespace Zwave
}  // namespace HubPlatform
