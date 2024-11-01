#pragma once

#include <string>

namespace HubPlatform {
namespace Zigbee {

enum class IasAceArmCommandMode {
    Disarm = 0x00,
    ArmDayHomeZonesOnly = 0x01,
    ArmNightSleepZonesOnly = 0x02,
    ArmAllZones = 0x03,

    Unknown = 0xff
};

const char* toString( IasAceArmCommandMode id );
IasAceArmCommandMode iasAceArmCommandModeFromString( const std::string& id );

}  // namespace Zigbee
}  // namespace HubPlatform
