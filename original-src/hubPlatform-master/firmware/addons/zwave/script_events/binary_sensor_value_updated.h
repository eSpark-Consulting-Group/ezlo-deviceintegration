#pragma once

#include "zwave/script_events/value_event.h"

namespace HubPlatform {
namespace Zwave {

class BinarySensorValueUpdated : public ValueEvent {
public:
    BinarySensorValueUpdated( uint8_t nodeId, uint8_t channelId, uint8_t value, int sensorType );

    Json toJson() const final;

private:
    uint8_t m_value;
    int m_sensorType;
};

}  // namespace Zwave
}  // namespace HubPlatform
