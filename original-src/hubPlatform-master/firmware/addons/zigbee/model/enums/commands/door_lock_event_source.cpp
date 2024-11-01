#include "zigbee/model/enums/commands/door_lock_event_source.h"
#include "zigbee/resources/zigbee_resources.h"

#include <cassert>

namespace HubPlatform {
namespace Zigbee {

struct DoorLockEventSourceData {
    DoorLockEventSource id;
    const char* name;
};

static const DoorLockEventSourceData doorLockEventSourceData[] = {
    { DoorLockEventSource::Keypad, Resources::Fields::Command::Formats::DoorLock::EventSource::Keypad },
    { DoorLockEventSource::RF, Resources::Fields::Command::Formats::DoorLock::EventSource::RF },
    { DoorLockEventSource::ManualOrReserved,
      Resources::Fields::Command::Formats::DoorLock::EventSource::ManualOrReserved },
    { DoorLockEventSource::RFID, Resources::Fields::Command::Formats::DoorLock::EventSource::RFID }
};

const char* toString( DoorLockEventSource id ) {
    for( const auto& data : doorLockEventSourceData ) {
        if( data.id == id ) {
            return data.name;
        }
    }

    return Resources::Fields::Command::Formats::DoorLock::EventSource::Indeterminate;
}

DoorLockEventSource doorLockEventSourceFromString( const std::string& name ) {
    for( const auto& data : doorLockEventSourceData ) {
        if( name == data.name ) {
            return data.id;
        }
    }

    return DoorLockEventSource::Indeterminate;
}

}  // namespace Zigbee
}  // namespace HubPlatform
