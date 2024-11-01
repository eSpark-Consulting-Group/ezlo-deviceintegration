#include "cored/house_modes/script_events/house_mode_exit_time_started.h"
#include "cored/house_modes/resources/house_mode_resources.h"

namespace HubPlatform {
namespace CoreDHouseModes {
namespace ScriptEvents {

HouseModeExitTimeStarted::HouseModeExitTimeStarted( bool silent, std::chrono::seconds timestamp )
: BaseEvent( Resources::ScriptEvents::Name::HouseModeExitTimeStarted )
, m_silent( silent )
, m_timestamp( timestamp ) {}

Json HouseModeExitTimeStarted::toJson() const {
    auto result = BaseEvent::toJson();

    result[ "silent" ] = m_silent;
    result[ "timestamp" ] = m_timestamp.count();

    return result;
}

}  // namespace ScriptEvents
}  // namespace CoreDHouseModes
}  // namespace HubPlatform
