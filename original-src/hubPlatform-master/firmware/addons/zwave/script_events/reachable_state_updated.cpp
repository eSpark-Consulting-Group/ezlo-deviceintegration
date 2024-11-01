#include "zwave/script_events/reachable_state_updated.h"

#include "zwave/resources/zwave_resources.h"

namespace HubPlatform {
namespace Zwave {

ReachableStateUpdated::ReachableStateUpdated( uint8_t nodeId, bool value )
: NodeEvent( Resources::ScriptEvent::Name::ReachableStateUpdated, nodeId )
, m_value( value ) {}

Json ReachableStateUpdated::toJson() const {
    auto result = NodeEvent::toJson();
    result[ Resources::ScriptEvent::Param::Value ] = m_value;
    return result;
}
}  // namespace Zwave
}  // namespace HubPlatform
