#pragma once

#include <string>

namespace HubPlatform {
namespace Zigbee {

enum class ThermostatFanMode {
    Off = 0x00,
    Low = 0x01,
    Medium = 0x02,
    High = 0x03,
    On = 0x04,
    Auto = 0x05,

    Unknown = 0xff
};

const char* toString( ThermostatFanMode id );
ThermostatFanMode thermostatFanModeFromString( const std::string& id );

}  // namespace Zigbee
}  // namespace HubPlatform
