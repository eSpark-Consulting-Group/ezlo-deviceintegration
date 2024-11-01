#include "zigbee/script_events/node_event.h"
#include "zigbee/resources/zigbee_resources.h"

#include <cassert>

namespace HubPlatform {
namespace Zigbee {

NodeEvent::NodeEvent( std::string eventName, uint16_t nodeId )
: Services::ScriptEvent( std::move( eventName ) )
, m_nodeId( nodeId ) {}

Json NodeEvent::formatObject( Json&& object ) const {
    assert( object.find( "_id" ) != object.end() );
    object[ "id" ] = object[ "_id" ];
    object.erase( "_id" );

    return object;
}

Json NodeEvent::toJson() const {
    auto result = Services::ScriptEvent::toJson();
    result[ Resources::ScriptEvent::Param::NodeId ] = m_nodeId;

    return std::move( result );
}

}  // namespace Zigbee
}  // namespace HubPlatform
