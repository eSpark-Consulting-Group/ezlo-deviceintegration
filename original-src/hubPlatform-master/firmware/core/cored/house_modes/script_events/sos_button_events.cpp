#include "sos_button_events.h"
#include "cored/house_modes/resources/house_mode_resources.h"
#include "json_utils/json_wrapper.h"

namespace HubPlatform {
namespace CoreDHouseModes {
namespace ScriptEvents {

ProtectSosButtonEvent::ProtectSosButtonEvent( const std::string& eventName, std::string deviceId, std::string service )
: ScriptEvents::BaseEvent( eventName )
, m_deviceId( std::move( deviceId ) )
, m_service( std::move( service ) ) {}

Json ProtectSosButtonEvent::toJson() const {
    auto result = BaseEvent::toJson();
    result[ "device_id" ] = m_deviceId;
    result[ "service" ] = m_service;
    return result;
}

ProtectSosButtonAdded::ProtectSosButtonAdded( std::string deviceId, std::string service )
: ProtectSosButtonEvent( Resources::SosButtonEvents::ProtectSosButtonAdded,
                         std::move( deviceId ),
                         std::move( service ) ) {}

ProtectSosButtonUpdated::ProtectSosButtonUpdated( std::string deviceId, std::string service )
: ProtectSosButtonEvent( Resources::SosButtonEvents::ProtectSosButtonUpdated,
                         std::move( deviceId ),
                         std::move( service ) ) {}

ProtectSosButtonRemoved::ProtectSosButtonRemoved( std::string deviceId )
: ProtectSosButtonEvent( Resources::SosButtonEvents::ProtectSosButtonRemoved, std::move( deviceId ), "" ) {}

Json ProtectSosButtonRemoved::toJson() const {
    auto result = BaseEvent::toJson();
    result[ "device_id" ] = m_deviceId;
    return result;
}

}  // namespace ScriptEvents
}  // namespace CoreDHouseModes
}  // namespace HubPlatform
