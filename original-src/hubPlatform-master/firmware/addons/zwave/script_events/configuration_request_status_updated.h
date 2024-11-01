#pragma once

#include "zwave/script_events/node_event.h"
#include <znet/znet_request_status.hpp>

namespace HubPlatform {
namespace Zwave {

class ConfigurationRequestStatusUpdated : public NodeEvent {
public:
    ConfigurationRequestStatusUpdated( uint8_t nodeId, uint16_t parameterNumber, znet::RequestStatus::Enum status );

    Json toJson() const final;

private:
    uint16_t m_parameterNumber;
    znet::RequestStatus::Enum m_status;
};

}  // namespace Zwave
}  // namespace HubPlatform
