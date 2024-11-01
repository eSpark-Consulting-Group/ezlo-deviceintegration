#include "zigbee/script_events/node_rejoined.h"
#include "zigbee/resources/zigbee_resources.h"

namespace HubPlatform {
namespace Zigbee {

NodeRejoined::NodeRejoined( uint16_t nodeId )
: NodeEvent( Resources::ScriptEvent::Name::NodeRejoined, nodeId ) {}

}  // namespace Zigbee
}  // namespace HubPlatform
