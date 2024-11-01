#pragma once

#include "zwave/script_events/value_event.h"

namespace HubPlatform {
namespace Zwave {

class ThermostatModeUpdated final : public ValueEvent {
public:
    ThermostatModeUpdated( uint8_t nodeId, uint8_t channelId, uint8_t mode, std::string manufacturerSpecificData );

    Json toJson() const override;

private:
    const uint8_t m_mode;
    std::string m_manufacturerSpecificData;
};

}  // namespace Zwave
}  // namespace HubPlatform
