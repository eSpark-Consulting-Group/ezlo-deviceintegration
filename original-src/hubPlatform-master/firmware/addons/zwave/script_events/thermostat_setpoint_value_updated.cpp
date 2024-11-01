#include "zwave/script_events/thermostat_setpoint_value_updated.h"

#include "zwave/resources/zwave_resources.h"

#include <znet/znet_defs.h>

namespace HubPlatform {
namespace Zwave {

ThermostatSetpointValueUpdated::ThermostatSetpointValueUpdated( uint8_t nodeId,
                                                                uint8_t channelId,
                                                                uint8_t type,
                                                                uint16_t scale,
                                                                float value,
                                                                bool isSetpointRequested )
: ValueEvent{ Resources::ScriptEvent::Name::ThermostatSetpointValueUpdated,
              nodeId,
              ZNET_COMMAND_CLASS_THERMOSTAT_SETPOINT,
              channelId }
, m_type{ type }
, m_scale{ scale }
, m_value{ value }
, m_isSetpointRequested( isSetpointRequested ) {}

Json ThermostatSetpointValueUpdated::toJson() const {
    using namespace Resources::ScriptEvent;

    auto result = ValueEvent::toJson();
    result[ Param::Type ] = m_type;
    result[ Param::Scale ] = m_scale;
    result[ Param::Value ] = m_value;
    result[ Param::IsRequested ] = m_isSetpointRequested;
    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
