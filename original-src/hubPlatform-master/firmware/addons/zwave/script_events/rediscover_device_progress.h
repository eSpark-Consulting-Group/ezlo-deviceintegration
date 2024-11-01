#pragma once

#include "zwave/script_events/node_event.h"
#include <znet/optional.hpp>
#include <znet/znet_error_message.hpp>

namespace HubPlatform {
namespace Zwave {

class RediscoverDeviceProgress : public NodeEvent {
public:
    explicit RediscoverDeviceProgress( uint8_t nodeId,
                                       std::string status,
                                       znet::utils::optional< uint8_t > progress,
                                       znet::ErrorOpt errorDetails );

    Json toJson() const override;

private:
    std::string m_status;
    znet::utils::optional< uint8_t > m_progress;
    znet::ErrorOpt m_errorDetails;
};

}  // namespace Zwave
}  // namespace HubPlatform
