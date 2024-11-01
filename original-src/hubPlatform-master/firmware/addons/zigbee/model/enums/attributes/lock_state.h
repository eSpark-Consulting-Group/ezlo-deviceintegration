#pragma once

#include <string>

namespace HubPlatform {
namespace Zigbee {

// Door Lock cluster, LockState attribute
enum class LockState {
    NotFullyLocked = 0x00,
    Locked = 0x01,
    Unlocked = 0x02,

    Unknown = 0xff
};

const char* toString( LockState id );
LockState lockStateFromString( const std::string& id );

}  // namespace Zigbee
}  // namespace HubPlatform
