#include "zigbee/script_events/node_added.h"
#include "zigbee/resources/zigbee_resources.h"

namespace HubPlatform {
namespace Zigbee {

NodeAdded::NodeAdded( uint16_t nodeId )
: NodeEvent( Resources::ScriptEvent::Name::NodeAdded, nodeId ) {}

}  // namespace Zigbee
}  // namespace HubPlatform
