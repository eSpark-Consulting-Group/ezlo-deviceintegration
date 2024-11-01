#include "zigbee/script_events/reachable_state_updated.h"
#include "zigbee/resources/zigbee_resources.h"

namespace HubPlatform {
namespace Zigbee {

ReachableStateUpdated::ReachableStateUpdated( uint16_t nodeId, bool isReachable )
: NodeEvent( Resources::ScriptEvent::Name::ReachableStateUpdated, nodeId )
, m_isReachable( isReachable ) {}

Json ReachableStateUpdated::toJson() const {
    auto result = NodeEvent::toJson();

    result[ Resources::ScriptEvent::Param::Value ] = m_isReachable;

    return result;
}

}  // namespace Zigbee
}  // namespace HubPlatform
