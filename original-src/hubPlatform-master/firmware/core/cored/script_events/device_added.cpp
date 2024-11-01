#include "cored/script_events/device_added.h"
#include "cored/script_events/resources/script_event_resources.h"

namespace HubPlatform {
namespace CoreD {
namespace ScriptEvents {

DeviceAdded::DeviceAdded( const Json& json )
: BaseEvent( Name::DeviceAdded, json ) {}

}  // namespace ScriptEvents
}  // namespace CoreD
}  // namespace HubPlatform
