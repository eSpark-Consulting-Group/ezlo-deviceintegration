#pragma once

#include <string>

namespace HubPlatform {
namespace Zigbee {

// Door Lock cluster, ActuatorEnabled attribute
enum class DoorLockActuatorState {
    Disabled = 0x00,
    Enabled = 0x01,

    Unknown = 0xff
};

const char* toString( DoorLockActuatorState id );
DoorLockActuatorState doorLockActuatorStateFromString( const std::string& id );

}  // namespace Zigbee
}  // namespace HubPlatform
