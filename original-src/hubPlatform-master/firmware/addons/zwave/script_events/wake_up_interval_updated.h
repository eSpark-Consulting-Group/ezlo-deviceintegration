#pragma once

#include "zwave/script_events/node_event.h"

#include <chrono>

namespace HubPlatform {
namespace Zwave {

class WakeUpIntervalUpdated final : public NodeEvent {
public:
    WakeUpIntervalUpdated( uint8_t nodeId, const std::chrono::seconds& interval );

    Json toJson() const override;

private:
    std::chrono::seconds m_interval;
};

}  // namespace Zwave
}  // namespace HubPlatform
