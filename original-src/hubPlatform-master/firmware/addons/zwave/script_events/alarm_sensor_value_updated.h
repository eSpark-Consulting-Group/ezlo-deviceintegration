#pragma once

#include "zwave/script_events/value_event.h"
#include <znet/znet_alarm_sensor.hpp>

namespace HubPlatform {
namespace Zwave {

class AlarmSensorValueUpdated : public ValueEvent {
public:
    AlarmSensorValueUpdated( uint8_t nodeId,
                             uint8_t channelId,
                             znet::AlarmSensorType sensorType,
                             uint8_t value,
                             uint16_t delay );

    Json toJson() const override;

private:
    const znet::AlarmSensorType m_sensorType;
    const uint8_t m_value;
    const uint16_t m_delay;
};

}  // namespace Zwave
}  // namespace HubPlatform
