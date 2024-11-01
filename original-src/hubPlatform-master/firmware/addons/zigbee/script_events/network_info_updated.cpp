#include "zigbee/script_events/network_info_updated.h"
#include "zigbee/model/zigbee_attribute.h"
#include "zigbee/resources/zigbee_resources.h"

namespace HubPlatform {
namespace Zigbee {

NetworkInfoUpdated::NetworkInfoUpdated()
: Services::ScriptEvent( Resources::ScriptEvent::Name::NetworkInfoUpdated ) {}

}  // namespace Zigbee
}  // namespace HubPlatform
