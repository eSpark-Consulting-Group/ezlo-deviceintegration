#pragma once

#include "zwave/script_events/value_event.h"
#include <znet/enums/multilevel_sensor.hpp>

namespace HubPlatform {
namespace Zwave {

class MultilevelSensorValueUpdated : public ValueEvent {
public:
    MultilevelSensorValueUpdated( uint8_t nodeId,
                                  uint8_t channelId,
                                  float value,
                                  znet::MultilevelSensor::Type sensorType,
                                  std::uint8_t scale );

    Json toJson() const final;

private:
    float m_value;
    znet::MultilevelSensor::Type m_sensorType;
    std::uint8_t m_scale;
};

}  // namespace Zwave
}  // namespace HubPlatform
