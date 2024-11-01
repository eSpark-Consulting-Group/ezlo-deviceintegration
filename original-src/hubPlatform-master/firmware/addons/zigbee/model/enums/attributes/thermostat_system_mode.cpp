#include "zigbee/model/enums/attributes/thermostat_system_mode.h"
#include "zigbee/resources/zigbee_resources.h"

#include <cassert>

namespace HubPlatform {
namespace Zigbee {

struct SystemModeData {
    ThermostatSystemMode modeId;
    const char* name;
};

static const SystemModeData systemModeData[] = {
    { ThermostatSystemMode::Off, Resources::Fields::Attribute::Formats::ThermostatCluster::Off },
    { ThermostatSystemMode::Auto, Resources::Fields::Attribute::Formats::ThermostatCluster::SystemMode::Auto },
    { ThermostatSystemMode::Cool, Resources::Fields::Attribute::Formats::ThermostatCluster::Cool },
    { ThermostatSystemMode::Heat, Resources::Fields::Attribute::Formats::ThermostatCluster::Heat },
    { ThermostatSystemMode::EmergencyHeating,
      Resources::Fields::Attribute::Formats::ThermostatCluster::SystemMode::EmergencyHeating },
    { ThermostatSystemMode::Precooling,
      Resources::Fields::Attribute::Formats::ThermostatCluster::SystemMode::Precooling },
    { ThermostatSystemMode::FanOnly, Resources::Fields::Attribute::Formats::ThermostatCluster::SystemMode::FanOnly },
    { ThermostatSystemMode::Dry, Resources::Fields::Attribute::Formats::ThermostatCluster::SystemMode::Dry },
    { ThermostatSystemMode::Sleep, Resources::Fields::Attribute::Formats::ThermostatCluster::SystemMode::Sleep }
};

const char* toString( ThermostatSystemMode id ) {
    for( const auto& data : systemModeData ) {
        if( data.modeId == id ) {
            return data.name;
        }
    }

    assert( !"Unknown Thermostat System Mode" );

    return Resources::Fields::Attribute::Formats::Unknown;
}

ThermostatSystemMode thermostatSystemModeFromString( const std::string& name ) {
    for( const auto& data : systemModeData ) {
        if( name == data.name ) {
            return data.modeId;
        }
    }

    assert( !"Unknown Thermostat System Mode name" );

    return ThermostatSystemMode::Unknown;
}

}  // namespace Zigbee
}  // namespace HubPlatform
