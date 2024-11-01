#pragma once

#include <string>

namespace HubPlatform {
namespace Zigbee {

// Door Lock cluster, LockType attribute
enum class LockType {
    DeadBolt = 0x00,
    Magnetic = 0x01,
    Other = 0x02,
    Mortise = 0x03,
    Rim = 0x04,
    LatchBolt = 0x05,
    CylindricalLock = 0x06,
    TubularLock = 0x07,
    InterconnectedLock = 0x08,
    DeadLatch = 0x09,
    DoorFurniture = 0x0A,

    Unknown = 0xff
};

const char* toString( LockType id );
LockType lockTypeFromString( const std::string& id );

}  // namespace Zigbee
}  // namespace HubPlatform
