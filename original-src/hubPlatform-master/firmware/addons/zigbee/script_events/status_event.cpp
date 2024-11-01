#include "zigbee/script_events/status_event.h"

#include "zigbee/resources/zigbee_resources.h"
#include "zigbee/resources/enums/mode_status.h"

namespace HubPlatform {
namespace Zigbee {

StatusEvent::StatusEvent( std::string eventName, ModeStatus status )
: Services::ScriptEvent( std::move( eventName ) )
, m_status( status ) {}

Json StatusEvent::toJson() const {
    using namespace Resources::ScriptEvent;

    auto result = Services::ScriptEvent::toJson();
    result[ Param::Status ] = toString( m_status );
    return result;
}

}  // namespace Zigbee
}  // namespace HubPlatform
