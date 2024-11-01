#include "cored/script_events/base_event.h"
#include "cored/resources/core_resources.h"

namespace HubPlatform {
namespace CoreD {
namespace ScriptEvents {

BaseEvent::BaseEvent( const std::string& event, const Json& json )
: Services::ScriptEvent( event, Resources::ModuleNames::Core )
, m_json( json ) {
    auto base = Services::ScriptEvent::toJson();
    for( auto it = base.begin(); it != base.end(); ++it ) {
        m_json[ it.key() ] = it.value();
    }
    m_json.erase( "syncNotification" );
    m_json.erase( "userNotification" );
    m_json.erase( "serviceNotification" );
}

Json BaseEvent::toJson() const {
    return m_json;
}

}  // namespace ScriptEvents
}  // namespace CoreD
}  // namespace HubPlatform
