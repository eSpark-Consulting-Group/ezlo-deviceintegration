#pragma once

#include <string>

namespace HubPlatform {
namespace Zigbee {

// Door Lock cluster, DoorState attribute
enum class DoorState {
    Open = 0x00,
    Closed = 0x01,
    ErrorJammed = 0x02,
    ErrorForcedOpen = 0x03,
    ErrorUnspecified = 0x04,

    Unknown = 0xff
};

const char* toString( DoorState id );
DoorState doorStateFromString( const std::string& id );

}  // namespace Zigbee
}  // namespace HubPlatform
