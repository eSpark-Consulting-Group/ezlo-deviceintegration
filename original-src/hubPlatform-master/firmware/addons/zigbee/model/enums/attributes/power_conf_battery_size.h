#pragma once

#include <string>

namespace HubPlatform {
namespace Zigbee {

enum class PowerConfBatterySize {
    NoBattery = 0x00,
    BuiltIn = 0x01,
    Other = 0x02,
    AA = 0x03,
    AAA = 0x04,
    C = 0x05,
    D = 0x06,
    CR2 = 0x07,
    CR123A = 0x08,

    Unknown = 0xff
};

const char* toString( PowerConfBatterySize id );
PowerConfBatterySize powerConfBatterySizeFromString( const std::string& id );

}  // namespace Zigbee
}  // namespace HubPlatform
