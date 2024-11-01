#include "cored/house_modes/script_events/house_mode_entry_delay_started.h"
#include "cored/house_modes/resources/house_mode_resources.h"

namespace HubPlatform {
namespace CoreDHouseModes {
namespace ScriptEvents {

HouseModeEntryDelayStarted::HouseModeEntryDelayStarted( bool silent,
                                                        bool chime,
                                                        std::string soundType,
                                                        std::chrono::seconds timestamp )
: BaseEvent( Resources::ScriptEvents::Name::HouseModeEntryDelayStarted )
, m_silent( silent )
, m_chime( chime )
, m_soundType( std::move( soundType ) )
, m_timestamp( timestamp ) {}

Json HouseModeEntryDelayStarted::toJson() const {
    auto result = BaseEvent::toJson();

    result[ "silent" ] = m_silent;
    result[ "chime" ] = m_chime;
    result[ "sound_type" ] = m_soundType;
    result[ "timestamp" ] = m_timestamp.count();

    return result;
}

}  // namespace ScriptEvents
}  // namespace CoreDHouseModes
}  // namespace HubPlatform
