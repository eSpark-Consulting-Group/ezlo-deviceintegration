#pragma once

#include "zwave/script_events/node_event.h"
#include <znet/optional.hpp>

namespace HubPlatform {
namespace Zwave {

class NodeFirmwareUpdateStatusEvent : public NodeEvent {
public:
    NodeFirmwareUpdateStatusEvent( uint8_t nodeId,
                                   znet::utils::optional< uint8_t > target,
                                   std::string status,
                                   znet::utils::optional< uint8_t > progress = {},
                                   std::string errorDescription = {} );

    Json toJson() const override;

private:
    znet::utils::optional< uint8_t > m_target;
    std::string m_status;
    znet::utils::optional< uint8_t > m_progress;
    std::string m_errorDescription;
};

}  // namespace Zwave
}  // namespace HubPlatform
