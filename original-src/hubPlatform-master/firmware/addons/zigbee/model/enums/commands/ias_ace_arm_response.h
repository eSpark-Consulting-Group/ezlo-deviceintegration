#pragma once

#include <string>

namespace HubPlatform {
namespace Zigbee {

enum class IasAceArmResponse {
    AllZonesDisarmed = 0x00,
    OnlyDayHomeZonesArmed = 0x01,
    OnlyNightSleepZonesArmed = 0x02,
    AllZonesArmed = 0x03,
    InvalidArmDisarmCode = 0x04,
    NotReadyToArm = 0x05,
    AlreadyDisarmed = 0x06,

    Unknown = 0xff
};

const char* toString( IasAceArmResponse id );
IasAceArmResponse iasAceArmResponseFromString( const std::string& id );

}  // namespace Zigbee
}  // namespace HubPlatform
