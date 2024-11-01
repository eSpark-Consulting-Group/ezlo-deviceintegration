#include "zwave/script_events/thermostat_setback_value_updated.h"

#include "zwave/resources/zwave_resources.h"

#include <znet/znet.h>

namespace HubPlatform {
namespace Zwave {

ThermostatSetbackValueUpdated::ThermostatSetbackValueUpdated( uint8_t nodeId,
                                                              uint8_t channelId,
                                                              std::string overrideType,
                                                              std::string mode,
                                                              znet::thermostat_setback::SetbackValueOpt valueOpt )
: ValueEvent( Resources::ScriptEvent::Name::ThermostatSetbackValueUpdated,
              nodeId,
              ZNET_COMMAND_CLASS_THERMOSTAT_SETBACK,
              channelId )
, m_overrideType( std::move( overrideType ) )
, m_mode( std::move( mode ) )
, m_valueOpt( valueOpt ) {}

Json ThermostatSetbackValueUpdated::toJson() const {
    using namespace Resources::ScriptEvent;

    Json result = ValueEvent::toJson();

    result[ Param::OverrideType ] = m_overrideType;
    result[ Param::Mode ] = m_mode;

    if( m_valueOpt ) {
        result[ Param::Value ] = *m_valueOpt;
    }

    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
