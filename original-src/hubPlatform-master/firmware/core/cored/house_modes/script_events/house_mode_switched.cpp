#include "cored/house_modes/script_events/house_mode_switched.h"
#include "cored/house_modes/resources/house_mode_resources.h"

namespace HubPlatform {
namespace CoreDHouseModes {
namespace ScriptEvents {

HouseModeSwitched::HouseModeSwitched( std::string from, std::string to, std::string status, int delay )
: BaseEvent( Resources::ScriptEvents::Name::HouseModeSwitched )
, m_from( std::move( from ) )
, m_to( std::move( to ) )
, m_status( std::move( status ) )
, m_delay( delay ) {}

Json HouseModeSwitched::toJson() const {
    auto result = BaseEvent::toJson();

    result[ "from" ] = m_from;
    result[ "to" ] = m_to;
    result[ "status" ] = m_status;
    result[ "switch_delay" ] = m_delay;

    return result;
}

}  // namespace ScriptEvents
}  // namespace CoreDHouseModes
}  // namespace HubPlatform
