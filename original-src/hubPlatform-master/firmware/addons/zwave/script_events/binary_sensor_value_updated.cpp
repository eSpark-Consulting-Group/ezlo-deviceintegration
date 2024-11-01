#include "zwave/script_events/binary_sensor_value_updated.h"

#include "zwave/resources/zwave_resources.h"

#include <znet/znet_defs.h>

namespace HubPlatform {
namespace Zwave {

BinarySensorValueUpdated::BinarySensorValueUpdated( uint8_t nodeId, uint8_t channelId, uint8_t value, int sensorType )
: ValueEvent( Resources::ScriptEvent::Name::BinarySensorValueUpdated,
              nodeId,
              ZNET_COMMAND_CLASS_SENSOR_BINARY,
              channelId )
, m_value( value )
, m_sensorType( sensorType ) {}

Json BinarySensorValueUpdated::toJson() const {
    using namespace Resources::ScriptEvent;

    auto result = ValueEvent::toJson();
    result[ Param::Value ] = m_value;
    result[ Param::SensorType ] = m_sensorType;
    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
