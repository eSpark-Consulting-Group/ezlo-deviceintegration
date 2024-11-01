#pragma once

#include <string>

namespace HubPlatform {
namespace Zigbee {

// Door Lock cluster, user related commands
enum class UserStatus {
    Available = 0x00,
    OccupiedEnabled = 0x01,
    OccupiedDisabled = 0x03,

    NotSupported = 0xff
};

const char* toString( UserStatus id );
UserStatus userStatusFromString( const std::string& id );

}  // namespace Zigbee
}  // namespace HubPlatform
