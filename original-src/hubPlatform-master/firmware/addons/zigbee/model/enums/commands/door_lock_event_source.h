#pragma once

#include <string>

namespace HubPlatform {
namespace Zigbee {

// Door Lock cluster, events
enum class DoorLockEventSource {
    Keypad = 0x00,
    RF = 0x01,
    ManualOrReserved = 0x02,
    RFID = 0x03,

    Indeterminate = 0xff
};

const char* toString( DoorLockEventSource id );
DoorLockEventSource doorLockEventSourceFromString( const std::string& id );

}  // namespace Zigbee
}  // namespace HubPlatform
