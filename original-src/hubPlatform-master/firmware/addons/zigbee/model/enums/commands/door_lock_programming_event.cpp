#include "zigbee/model/enums/commands/door_lock_programming_event.h"
#include "zigbee/resources/zigbee_resources.h"

#include <cassert>

namespace HubPlatform {
namespace Zigbee {

struct DoorLockProgrammingEventData {
    DoorLockProgrammingEvent id;
    const char* name;
};

static const DoorLockProgrammingEventData doorLockProgrammingEventData[] = {
    { DoorLockProgrammingEvent::UnknownOrMfgSpecific,
      Resources::Fields::Command::Formats::DoorLock::ProgrammingEvent::UnknownOrMfgSpecific },
    { DoorLockProgrammingEvent::MasterCodeChanged,
      Resources::Fields::Command::Formats::DoorLock::ProgrammingEvent::MasterCodeChanged },
    { DoorLockProgrammingEvent::PINCodeAdded,
      Resources::Fields::Command::Formats::DoorLock::ProgrammingEvent::PINCodeAdded },
    { DoorLockProgrammingEvent::PINCodeDeleted,
      Resources::Fields::Command::Formats::DoorLock::ProgrammingEvent::PINCodeDeleted },
    { DoorLockProgrammingEvent::PINCodeChanged,
      Resources::Fields::Command::Formats::DoorLock::ProgrammingEvent::PINCodeChanged },
    { DoorLockProgrammingEvent::RFIDCodeAdded,
      Resources::Fields::Command::Formats::DoorLock::ProgrammingEvent::RFIDCodeAdded },
    { DoorLockProgrammingEvent::RFIDCodeDeleted,
      Resources::Fields::Command::Formats::DoorLock::ProgrammingEvent::RFIDCodeDeleted }
};

const char* toString( DoorLockProgrammingEvent id ) {
    for( const auto& data : doorLockProgrammingEventData ) {
        if( data.id == id ) {
            return data.name;
        }
    }

    assert( !"Unknown Door Lock Programming Event" );

    return Resources::Fields::Command::Formats::Unknown;
}

DoorLockProgrammingEvent doorLockProgrammingEventFromString( const std::string& name ) {
    for( const auto& data : doorLockProgrammingEventData ) {
        if( name == data.name ) {
            return data.id;
        }
    }

    assert( !"Unknown Door Lock Programming Event name" );

    return DoorLockProgrammingEvent::Unknown;
}

}  // namespace Zigbee
}  // namespace HubPlatform
