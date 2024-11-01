#pragma once

#include <string>

namespace HubPlatform {
namespace Zigbee {

// Door Lock cluster, user related commands
enum class UserType {
    UnrestrictedUser = 0x00,
    YearDayScheduleUser = 0x01,
    WeekDayScheduleUser = 0x02,
    MasterUser = 0x03,
    NonAccessUser = 0x04,

    NotSupported = 0xff
};

const char* toString( UserType id );
UserType userTypeFromString( const std::string& id );

}  // namespace Zigbee
}  // namespace HubPlatform
