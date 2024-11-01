#pragma once

#include <string>

namespace HubPlatform {
namespace Zigbee {

// Door Lock cluster, programming events
enum class DoorLockProgrammingEvent {
    UnknownOrMfgSpecific = 0x00,
    MasterCodeChanged = 0x01,
    PINCodeAdded = 0x02,
    PINCodeDeleted = 0x03,
    PINCodeChanged = 0x04,
    RFIDCodeAdded = 0x05,
    RFIDCodeDeleted = 0x06,

    Unknown = 0xff
};

const char* toString( DoorLockProgrammingEvent id );
DoorLockProgrammingEvent doorLockProgrammingEventFromString( const std::string& id );

}  // namespace Zigbee
}  // namespace HubPlatform
