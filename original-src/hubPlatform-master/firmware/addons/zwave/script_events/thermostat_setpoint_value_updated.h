#pragma once

#include "zwave/script_events/value_event.h"

namespace HubPlatform {
namespace Zwave {

class ThermostatSetpointValueUpdated final : public ValueEvent {
public:
    ThermostatSetpointValueUpdated( uint8_t nodeId,
                                    uint8_t channelId,
                                    uint8_t type,
                                    uint16_t scale,
                                    float value,
                                    bool isSetpointRequested );

    Json toJson() const override;

private:
    const uint8_t m_type;
    const uint16_t m_scale;
    const float m_value;
    const bool m_isSetpointRequested;
};

}  // namespace Zwave
}  // namespace HubPlatform
