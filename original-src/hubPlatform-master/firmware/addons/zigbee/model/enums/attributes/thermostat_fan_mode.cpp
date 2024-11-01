#include "zigbee/model/enums/attributes/thermostat_fan_mode.h"
#include "zigbee/resources/zigbee_resources.h"

#include <cassert>

namespace HubPlatform {
namespace Zigbee {

struct FanModeData {
    ThermostatFanMode modeId;
    const char* name;
};

static const FanModeData fanModeData[] = {
    { ThermostatFanMode::Off, Resources::Fields::Attribute::Formats::FanControlCluster::Off },
    { ThermostatFanMode::Low, Resources::Fields::Attribute::Formats::FanControlCluster::Low },
    { ThermostatFanMode::Medium, Resources::Fields::Attribute::Formats::FanControlCluster::Medium },
    { ThermostatFanMode::High, Resources::Fields::Attribute::Formats::FanControlCluster::High },
    { ThermostatFanMode::On, Resources::Fields::Attribute::Formats::FanControlCluster::On },
    { ThermostatFanMode::Auto, Resources::Fields::Attribute::Formats::FanControlCluster::Auto }
};

const char* toString( ThermostatFanMode id ) {
    for( const auto& data : fanModeData ) {
        if( data.modeId == id ) {
            return data.name;
        }
    }

    assert( !"Unknown Thermostat System Mode" );

    return Resources::Fields::Attribute::Formats::Unknown;
}

ThermostatFanMode thermostatFanModeFromString( const std::string& name ) {
    for( const auto& data : fanModeData ) {
        if( name == data.name ) {
            return data.modeId;
        }
    }

    assert( !"Unknown Thermostat Fan Mode name" );

    return ThermostatFanMode::Unknown;
}

}  // namespace Zigbee
}  // namespace HubPlatform
