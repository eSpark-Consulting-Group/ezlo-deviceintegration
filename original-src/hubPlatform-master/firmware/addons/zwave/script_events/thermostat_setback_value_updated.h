#pragma once

#include "zwave/script_events/value_event.h"

#include <znet/enums/thermostat_setback.hpp>

namespace HubPlatform {
namespace Zwave {

class ThermostatSetbackValueUpdated final : public ValueEvent {
public:
    ThermostatSetbackValueUpdated( uint8_t nodeId,
                                   uint8_t channelId,
                                   std::string overrideType,
                                   std::string mode,
                                   znet::thermostat_setback::SetbackValueOpt valueOpt );

    Json toJson() const override;

private:
    std::string m_overrideType;
    std::string m_mode;
    znet::thermostat_setback::SetbackValueOpt m_valueOpt;
};

}  // namespace Zwave
}  // namespace HubPlatform
