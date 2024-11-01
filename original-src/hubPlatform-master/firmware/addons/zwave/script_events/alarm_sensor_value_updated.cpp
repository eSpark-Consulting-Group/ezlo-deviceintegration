#include "zwave/script_events/alarm_sensor_value_updated.h"

#include "zwave/resources/zwave_resources.h"

#include <znet/znet_defs.h>

namespace HubPlatform {
namespace Zwave {

AlarmSensorValueUpdated::AlarmSensorValueUpdated( uint8_t nodeId,
                                                  uint8_t channelId,
                                                  znet::AlarmSensorType sensorType,
                                                  uint8_t value,
                                                  uint16_t delay )
: ValueEvent( Resources::ScriptEvent::Name::AlarmSensorValueUpdated,
              nodeId,
              ZNET_COMMAND_CLASS_SENSOR_ALARM,
              channelId )
, m_sensorType( sensorType )
, m_value( value )
, m_delay( delay ) {}

Json AlarmSensorValueUpdated::toJson() const {
    auto result = ValueEvent::toJson();

    result[ Resources::ScriptEvent::Param::Value ] = m_value;
    result[ Resources::ScriptEvent::Param::Delay ] = m_delay;
    result[ Resources::ScriptEvent::Param::SensorType ] = znet::getAlarmSensorName( m_sensorType );

    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
