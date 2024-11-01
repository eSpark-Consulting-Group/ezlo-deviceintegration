#include "zigbee/model/enums/commands/user_type.h"
#include "zigbee/resources/zigbee_resources.h"

#include <cassert>

namespace HubPlatform {
namespace Zigbee {

struct UserTypeData {
    UserType id;
    const char* name;
};

static const UserTypeData userTypeData[] = {
    { UserType::UnrestrictedUser, Resources::Fields::Command::Formats::DoorLock::UserType::UnrestrictedUser },
    { UserType::YearDayScheduleUser, Resources::Fields::Command::Formats::DoorLock::UserType::YearDayScheduleUser },
    { UserType::WeekDayScheduleUser, Resources::Fields::Command::Formats::DoorLock::UserType::WeekDayScheduleUser },
    { UserType::MasterUser, Resources::Fields::Command::Formats::DoorLock::UserType::MasterUser },
    { UserType::NonAccessUser, Resources::Fields::Command::Formats::DoorLock::UserType::NonAccessUser }
};

const char* toString( UserType id ) {
    for( const auto& data : userTypeData ) {
        if( data.id == id ) {
            return data.name;
        }
    }

    return Resources::Fields::Command::Formats::DoorLock::NotSupported;
}

UserType userTypeFromString( const std::string& name ) {
    for( const auto& data : userTypeData ) {
        if( name == data.name ) {
            return data.id;
        }
    }

    return UserType::NotSupported;
}

}  // namespace Zigbee
}  // namespace HubPlatform
