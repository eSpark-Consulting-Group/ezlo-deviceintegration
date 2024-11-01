#include "zigbee/model/enums/attributes/power_conf_battery_size.h"
#include "zigbee/resources/zigbee_resources.h"

#include <cassert>

namespace HubPlatform {
namespace Zigbee {

struct BatterySizeData {
    PowerConfBatterySize id;
    const char* name;
};

static const BatterySizeData batterySizeData[] = {
    { PowerConfBatterySize::NoBattery,
      Resources::Fields::Attribute::Formats::PowerConfigurationCluster::BatterySize::NoBattery },
    { PowerConfBatterySize::BuiltIn,
      Resources::Fields::Attribute::Formats::PowerConfigurationCluster::BatterySize::BuiltIn },
    { PowerConfBatterySize::Other,
      Resources::Fields::Attribute::Formats::PowerConfigurationCluster::BatterySize::Other },
    { PowerConfBatterySize::AA, Resources::Fields::Attribute::Formats::PowerConfigurationCluster::BatterySize::AA },
    { PowerConfBatterySize::AAA, Resources::Fields::Attribute::Formats::PowerConfigurationCluster::BatterySize::AAA },
    { PowerConfBatterySize::C, Resources::Fields::Attribute::Formats::PowerConfigurationCluster::BatterySize::C },
    { PowerConfBatterySize::D, Resources::Fields::Attribute::Formats::PowerConfigurationCluster::BatterySize::D },
    { PowerConfBatterySize::CR2, Resources::Fields::Attribute::Formats::PowerConfigurationCluster::BatterySize::CR2 },
    { PowerConfBatterySize::CR123A,
      Resources::Fields::Attribute::Formats::PowerConfigurationCluster::BatterySize::CR123A },
    { PowerConfBatterySize::Unknown, Resources::Fields::Attribute::Formats::Unknown }
};

const char* toString( PowerConfBatterySize id ) {
    for( const auto& data : batterySizeData ) {
        if( data.id == id ) {
            return data.name;
        }
    }

    assert( !"Unknown Battery Size" );

    return Resources::Fields::Attribute::Formats::Unknown;
}

PowerConfBatterySize powerConfBatterySizeFromString( const std::string& name ) {
    for( const auto& data : batterySizeData ) {
        if( name == data.name ) {
            return data.id;
        }
    }

    assert( !"Unknown Battery Size name" );

    return PowerConfBatterySize::Unknown;
}

}  // namespace Zigbee
}  // namespace HubPlatform
