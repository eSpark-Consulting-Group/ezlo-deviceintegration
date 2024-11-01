#include "zwave/script_events/thermostat_mode_updated.h"

#include "zwave/resources/zwave_resources.h"

#include <znet/znet_defs.h>

namespace HubPlatform {
namespace Zwave {

ThermostatModeUpdated::ThermostatModeUpdated( uint8_t nodeId,
                                              uint8_t channelId,
                                              uint8_t mode,
                                              std::string manufacturerSpecificData )
: ValueEvent{ Resources::ScriptEvent::Name::ThermostatModeUpdated,
              nodeId,
              ZNET_COMMAND_CLASS_THERMOSTAT_MODE,
              channelId }
, m_mode{ mode }
, m_manufacturerSpecificData{ std::move( manufacturerSpecificData ) } {}

Json ThermostatModeUpdated::toJson() const {
    using namespace Resources::ScriptEvent;

    auto result = ValueEvent::toJson();
    result[ Param::Mode ] = m_mode;

    if( !m_manufacturerSpecificData.empty() ) {
        result[ Param::ManufacturerSpecificData ] = m_manufacturerSpecificData;
    }

    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
