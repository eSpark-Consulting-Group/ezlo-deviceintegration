#pragma once

#include "zwave/script_events/value_event.h"

namespace HubPlatform {
namespace Zwave {

class BarrierSignalValueUpdated : public ValueEvent {
public:
    BarrierSignalValueUpdated( uint8_t nodeId, uint8_t channelId, uint8_t signalId, uint8_t value );

    Json toJson() const final;

private:
    uint8_t m_signalId;
    uint8_t m_value;
};

}  // namespace Zwave
}  // namespace HubPlatform
