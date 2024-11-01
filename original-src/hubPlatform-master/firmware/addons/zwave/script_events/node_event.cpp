#include "zwave/script_events/node_event.h"

#include "zwave/resources/zwave_resources.h"

namespace HubPlatform {
namespace Zwave {

NodeEvent::NodeEvent( std::string eventName, uint8_t nodeId )
: Services::ScriptEvent( std::move( eventName ) )
, m_nodeId( nodeId ) {}

Json NodeEvent::toJson() const {
    auto result = Services::ScriptEvent::toJson();
    result[ Resources::ScriptEvent::Param::NodeId ] = m_nodeId;
    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
