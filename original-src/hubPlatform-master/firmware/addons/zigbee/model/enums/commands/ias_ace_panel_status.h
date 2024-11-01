#pragma once

#include <string>

namespace HubPlatform {
namespace Zigbee {

enum class IasAcePanelStatus {
    PanelDisarmed = 0x00,
    ArmedStay = 0x01,
    ArmedNight = 0x02,
    ArmedAway = 0x03,
    ExitDelay = 0x04,
    EntryDelay = 0x05,
    NotReadyToArm = 0x06,
    InAlarm = 0x07,
    ArmingStay = 0x08,
    ArmingNight = 0x09,
    ArmingAway = 0x0A,

    Unknown = 0xff
};

const char* toString( IasAcePanelStatus id );
IasAcePanelStatus iasAcePanelStatusFromString( const std::string& id );

}  // namespace Zigbee
}  // namespace HubPlatform
