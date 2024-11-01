#include "zigbee/script_events/module_reset.h"
#include "zigbee/resources/zigbee_resources.h"

namespace HubPlatform {
namespace Zigbee {

ModuleReset::ModuleReset( ModeStatus status )
: StatusEvent( Resources::ScriptEvent::Name::ModuleReset, status ) {}

}  // namespace Zigbee
}  // namespace HubPlatform
