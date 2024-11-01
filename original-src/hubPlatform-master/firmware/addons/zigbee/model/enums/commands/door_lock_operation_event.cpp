#include "zigbee/model/enums/commands/door_lock_operation_event.h"
#include "zigbee/resources/zigbee_resources.h"

#include <cassert>

namespace HubPlatform {
namespace Zigbee {

struct DoorLockOperationEventData {
    DoorLockOperationEvent id;
    const char* name;
};

static const DoorLockOperationEventData doorLockOperationEventData[] = {
    { DoorLockOperationEvent::UnknownOrMfgSpecific,
      Resources::Fields::Command::Formats::DoorLock::OperationEvent::UnknownOrMfgSpecific },
    { DoorLockOperationEvent::Lock, Resources::Fields::Command::Formats::DoorLock::OperationEvent::Lock },
    { DoorLockOperationEvent::Unlock, Resources::Fields::Command::Formats::DoorLock::OperationEvent::Unlock },
    { DoorLockOperationEvent::LockFailureInvalidPINorID,
      Resources::Fields::Command::Formats::DoorLock::OperationEvent::LockFailureInvalidPINorID },
    { DoorLockOperationEvent::LockFailureInvalidSchedule,
      Resources::Fields::Command::Formats::DoorLock::OperationEvent::LockFailureInvalidSchedule },
    { DoorLockOperationEvent::UnlockFailureInvalidPINorID,
      Resources::Fields::Command::Formats::DoorLock::OperationEvent::UnlockFailureInvalidPINorID },
    { DoorLockOperationEvent::UnlockFailureInvalidSchedule,
      Resources::Fields::Command::Formats::DoorLock::OperationEvent::UnlockFailureInvalidSchedule },
    { DoorLockOperationEvent::OneTouchLock,
      Resources::Fields::Command::Formats::DoorLock::OperationEvent::OneTouchLock },
    { DoorLockOperationEvent::KeyLock, Resources::Fields::Command::Formats::DoorLock::OperationEvent::KeyLock },
    { DoorLockOperationEvent::KeyUnlock, Resources::Fields::Command::Formats::DoorLock::OperationEvent::KeyUnlock },
    { DoorLockOperationEvent::AutoLock, Resources::Fields::Command::Formats::DoorLock::OperationEvent::AutoLock },
    { DoorLockOperationEvent::ScheduleLock,
      Resources::Fields::Command::Formats::DoorLock::OperationEvent::ScheduleLock },
    { DoorLockOperationEvent::ScheduleUnlock,
      Resources::Fields::Command::Formats::DoorLock::OperationEvent::ScheduleUnlock },
    { DoorLockOperationEvent::ManualLock, Resources::Fields::Command::Formats::DoorLock::OperationEvent::ManualLock },
    { DoorLockOperationEvent::ManualUnlock,
      Resources::Fields::Command::Formats::DoorLock::OperationEvent::ManualUnlock },
    { DoorLockOperationEvent::NonAccessUserOperationalEvent,
      Resources::Fields::Command::Formats::DoorLock::OperationEvent::NonAccessUserOperationalEvent }
};

const char* toString( DoorLockOperationEvent id ) {
    for( const auto& data : doorLockOperationEventData ) {
        if( data.id == id ) {
            return data.name;
        }
    }

    assert( !"Unknown Door Lock Operation Event" );

    return Resources::Fields::Command::Formats::Unknown;
}

DoorLockOperationEvent doorLockOperationEventFromString( const std::string& name ) {
    for( const auto& data : doorLockOperationEventData ) {
        if( name == data.name ) {
            return data.id;
        }
    }

    assert( !"Unknown Door Lock Operation Event name" );

    return DoorLockOperationEvent::Unknown;
}

}  // namespace Zigbee
}  // namespace HubPlatform
