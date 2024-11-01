#include "house_mode_entry_delay_finished.h"

namespace HubPlatform {
namespace CoreDHouseModes {
namespace ScriptEvents {

HouseModeEntryDelayFinished::HouseModeEntryDelayFinished( bool silent,
                                                          bool chime,
                                                          std::string soundType,
                                                          std::chrono::seconds timestamp,
                                                          Resources::EntryDelayStatus status )
: BaseEvent( Resources::ScriptEvents::Name::HouseModeEntryDelayFinished )
, m_silent( silent )
, m_chime( chime )
, m_soundType( std::move( soundType ) )
, m_timestamp( timestamp )
, m_status( status ) {}

Json HouseModeEntryDelayFinished::toJson() const {
    auto result = BaseEvent::toJson();

    result[ "silent" ] = m_silent;
    result[ "chime" ] = m_chime;
    result[ "sound_type" ] = m_soundType;
    result[ "timestamp" ] = m_timestamp.count();
    result[ "status" ] = enumToString();

    return result;
}

std::string HouseModeEntryDelayFinished::enumToString() const {
    if( m_status == Resources::EntryDelayStatus::DONE ) {
        return "done";
    }
    else if( m_status == Resources::EntryDelayStatus::CANCELED ) {
        return "canceled";
    }
    else if( m_status == Resources::EntryDelayStatus::SKIPPED ) {
        return "skipped";
    }
    return "";
}

}  // namespace ScriptEvents
}  // namespace CoreDHouseModes
}  // namespace HubPlatform
