#include "cored/script_events/device_updated.h"
#include "cored/script_events/resources/script_event_resources.h"

namespace HubPlatform {
namespace CoreD {
namespace ScriptEvents {

DeviceUpdated::DeviceUpdated( const Json& json )
: BaseEvent( Name::DeviceUpdated, json ) {}

}  // namespace ScriptEvents
}  // namespace CoreD
}  // namespace HubPlatform
