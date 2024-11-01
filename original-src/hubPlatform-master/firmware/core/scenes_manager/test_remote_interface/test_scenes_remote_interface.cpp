#include "scenes_manager/test_remote_interface/test_scenes_remote_interface.h"

namespace HubPlatform {
namespace ScenesManager {

TestRemoteInterface::TestRemoteInterface( Services::Service& hostService )
: m_hostService( hostService ) {}

JsonObject TestRemoteInterface::setTestClock( std::chrono::seconds startUnixTime, std::chrono::hours timeZoneOffset ) {
    const Json request =
        JSON( "startUnixTime" << startUnixTime.count() << "timeZoneOffset" << timeZoneOffset.count() << "units"
                              << "hour" );
    return m_hostService.sendRequest( GroupsNames::SCENE_MANAGER, request, SCENE_SET_TEST_CLOCK );
}

JsonObject TestRemoteInterface::setTestClock( std::chrono::seconds startUnixTime,
                                              std::chrono::minutes timeZoneOffsetMinutes ) {
    const Json request =
        JSON( "startUnixTime" << startUnixTime.count() << "timeZoneOffset" << timeZoneOffsetMinutes.count() << "units"
                              << "minute" );
    return m_hostService.sendRequest( GroupsNames::SCENE_MANAGER, request, SCENE_SET_TEST_CLOCK );
}

}  // namespace ScenesManager
}  // namespace HubPlatform
