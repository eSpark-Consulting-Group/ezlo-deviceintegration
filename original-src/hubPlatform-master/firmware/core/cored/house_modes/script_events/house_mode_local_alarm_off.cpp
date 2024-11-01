#include "cored/house_modes/script_events/house_mode_local_alarm_off.h"
#include "cored/house_modes/resources/house_mode_resources.h"

namespace HubPlatform {
namespace CoreDHouseModes {
namespace ScriptEvents {

HouseModeLocalAlarmOff::HouseModeLocalAlarmOff( std::string& modeId, std::chrono::seconds timestamp )
: BaseEvent( Resources::ScriptEvents::Name::HouseModeLocalAlarmsTurnedOff )
, m_modeId( std::move( modeId ) )
, m_timestamp( timestamp ) {}

Json HouseModeLocalAlarmOff::toJson() const {
    auto result = BaseEvent::toJson();

    result[ "house_mode_id" ] = m_modeId;
    result[ "timestamp" ] = m_timestamp.count();

    return result;
}

}  // namespace ScriptEvents
}  // namespace CoreDHouseModes
}  // namespace HubPlatform
