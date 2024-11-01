#include "zwave/script_events/thermostat_operating_state_value_updated.h"

#include "zwave/resources/zwave_resources.h"

#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace Zwave {

ThermostatOperatingStateValueUpdated::ThermostatOperatingStateValueUpdated( uint8_t nodeId,
                                                                            uint8_t channelId,
                                                                            uint8_t classId,
                                                                            std::string operatingState )
: ValueEvent{ Resources::ScriptEvent::Name::ThermostatOperatingStateValueUpdated, nodeId, classId, channelId }
, m_operatingState{ std::move( operatingState ) } {}

ThermostatOperatingStateValueUpdated::~ThermostatOperatingStateValueUpdated() = default;

Json ThermostatOperatingStateValueUpdated::toJson() const {
    using namespace Resources::ScriptEvent;

    auto result = ValueEvent::toJson();
    json_add_str( result, Param::Value, m_operatingState );
    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
