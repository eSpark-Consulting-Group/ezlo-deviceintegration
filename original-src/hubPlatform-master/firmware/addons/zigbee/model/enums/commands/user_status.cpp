#include "zigbee/model/enums/commands/user_status.h"
#include "zigbee/resources/zigbee_resources.h"

#include <cassert>

namespace HubPlatform {
namespace Zigbee {

struct UserStatusData {
    UserStatus id;
    const char* name;
};

static const UserStatusData userStatusData[] = {
    { UserStatus::Available, Resources::Fields::Command::Formats::DoorLock::UserStatus::Available },
    { UserStatus::OccupiedEnabled, Resources::Fields::Command::Formats::DoorLock::UserStatus::OccupiedEnabled },
    { UserStatus::OccupiedDisabled, Resources::Fields::Command::Formats::DoorLock::UserStatus::OccupiedDisabled }
};

const char* toString( UserStatus id ) {
    for( const auto& data : userStatusData ) {
        if( data.id == id ) {
            return data.name;
        }
    }

    return Resources::Fields::Command::Formats::DoorLock::NotSupported;
}

UserStatus userStatusFromString( const std::string& name ) {
    for( const auto& data : userStatusData ) {
        if( name == data.name ) {
            return data.id;
        }
    }

    return UserStatus::NotSupported;
}

}  // namespace Zigbee
}  // namespace HubPlatform
