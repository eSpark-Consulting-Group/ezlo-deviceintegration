#pragma once

#include <string>

namespace HubPlatform {
namespace Zigbee {

enum class ThermostatSystemMode {
    Off = 0x00,
    Auto = 0x01,
    Cool = 0x03,
    Heat = 0x04,
    EmergencyHeating = 0x05,
    Precooling = 0x06,
    FanOnly = 0x07,
    Dry = 0x08,
    Sleep = 0x09,

    Unknown = 0xff
};

const char* toString( ThermostatSystemMode id );
ThermostatSystemMode thermostatSystemModeFromString( const std::string& id );

}  // namespace Zigbee
}  // namespace HubPlatform
