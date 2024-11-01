#include "zwave/script_events/multilevel_sensor_value_updated.h"

#include "zwave/resources/zwave_resources.h"

#include <znet/znet_defs.h>
#include <znet/converters/multilevel_sensor_converter.hpp>

namespace HubPlatform {
namespace Zwave {

MultilevelSensorValueUpdated::MultilevelSensorValueUpdated( uint8_t nodeId,
                                                            uint8_t channelId,
                                                            float value,
                                                            znet::MultilevelSensor::Type sensorType,
                                                            std::uint8_t scale )
: ValueEvent( Resources::ScriptEvent::Name::MultilevelSensorValueUpdated,
              nodeId,
              ZNET_COMMAND_CLASS_SENSOR_MULTILEVEL,
              channelId )
, m_value( value )
, m_sensorType( sensorType )
, m_scale( scale ) {}

Json MultilevelSensorValueUpdated::toJson() const {
    using namespace Resources::ScriptEvent;

    auto result = ValueEvent::toJson();
    result[ Param::Value ] = m_value;
    addOptionalStringField( result, Param::SensorType, znet::multilevel_sensor_converter::typeToString, m_sensorType );
    addOptionalStringField(
        result, Param::Scale, znet::multilevel_sensor_converter::scaleToString, m_sensorType, m_scale );
    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
