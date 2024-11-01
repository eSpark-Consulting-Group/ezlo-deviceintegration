#include "zigbee/script_events/include_device.h"
#include "zigbee/resources/zigbee_resources.h"

namespace HubPlatform {
namespace Zigbee {

IncludeDevice::IncludeDevice( ModeStatus status )
: StatusEvent( Resources::ScriptEvent::Name::IncludeDevice, status ) {}

}  // namespace Zigbee
}  // namespace HubPlatform
