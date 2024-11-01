#pragma once

#include "zwave/script_events/value_event.h"

namespace HubPlatform {
namespace Zwave {

class ThermostatOperatingStateValueUpdated final : public ValueEvent {
public:
    ThermostatOperatingStateValueUpdated( uint8_t nodeId,
                                          uint8_t channelId,
                                          uint8_t classId,
                                          std::string operatingState );

    ~ThermostatOperatingStateValueUpdated() override;

    Json toJson() const override;

private:
    std::string m_operatingState;
};

}  // namespace Zwave
}  // namespace HubPlatform
