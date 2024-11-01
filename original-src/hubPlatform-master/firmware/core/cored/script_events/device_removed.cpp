#include "cored/script_events/device_removed.h"
#include "cored/script_events/resources/script_event_resources.h"

namespace HubPlatform {
namespace CoreD {
namespace ScriptEvents {

DeviceRemoved::DeviceRemoved( const Json& json )
: BaseEvent( Name::DeviceRemoved, json ) {}

}  // namespace ScriptEvents
}  // namespace CoreD
}  // namespace HubPlatform
