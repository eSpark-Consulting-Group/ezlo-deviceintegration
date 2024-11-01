#include "cored/house_modes/script_events/house_mode_alarmed.h"
#include "cored/house_modes/resources/house_mode_resources.h"

namespace HubPlatform {
namespace CoreDHouseModes {
namespace ScriptEvents {

HouseModeAlarmed::HouseModeAlarmed( std::string modeId,
                                    std::string deviceId,
                                    std::string phase,
                                    std::string status,
                                    int delay,
                                    bool silent,
                                    bool chime,
                                    std::string soundType )
: BaseEvent( Resources::ScriptEvents::Name::HouseModeAlarmed )
, m_modeId( std::move( modeId ) )
, m_deviceId( std::move( deviceId ) )
, m_phase( std::move( phase ) )
, m_status( std::move( status ) )
, m_delay( delay )
, m_silent( silent )
, m_chime( chime )
, m_soundType( std::move( soundType ) ) {}

Json HouseModeAlarmed::toJson() const {
    auto result = BaseEvent::toJson();

    result[ "house_mode_id" ] = m_modeId;
    result[ "device_id" ] = m_deviceId;
    result[ "phase" ] = m_phase;
    result[ "status" ] = m_status;
    result[ "delay" ] = m_delay;
    result[ "silent" ] = m_silent;
    result[ "chime" ] = m_chime;
    result[ "sound_type" ] = m_soundType;

    return result;
}

}  // namespace ScriptEvents
}  // namespace CoreDHouseModes
}  // namespace HubPlatform
