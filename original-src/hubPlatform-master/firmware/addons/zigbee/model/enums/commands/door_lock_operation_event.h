#pragma once

#include <string>

namespace HubPlatform {
namespace Zigbee {

// Door Lock cluster, operation events
enum class DoorLockOperationEvent {
    UnknownOrMfgSpecific = 0x00,
    Lock = 0x01,
    Unlock = 0x02,
    LockFailureInvalidPINorID = 0x03,
    LockFailureInvalidSchedule = 0x04,
    UnlockFailureInvalidPINorID = 0x05,
    UnlockFailureInvalidSchedule = 0x06,
    OneTouchLock = 0x07,
    KeyLock = 0x08,
    KeyUnlock = 0x09,
    AutoLock = 0x0A,
    ScheduleLock = 0x0B,
    ScheduleUnlock = 0x0C,
    ManualLock = 0x0D,
    ManualUnlock = 0x0E,
    NonAccessUserOperationalEvent = 0x0F,

    Unknown = 0xff
};

const char* toString( DoorLockOperationEvent id );
DoorLockOperationEvent doorLockOperationEventFromString( const std::string& id );

}  // namespace Zigbee
}  // namespace HubPlatform
