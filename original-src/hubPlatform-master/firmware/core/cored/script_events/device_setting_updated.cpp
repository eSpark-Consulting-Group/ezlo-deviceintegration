#include "cored/script_events/device_setting_updated.h"
#include "cored/script_events/resources/script_event_resources.h"

namespace HubPlatform {
namespace CoreD {
namespace ScriptEvents {

DeviceSettingUpdated::DeviceSettingUpdated( const Json& json )
: BaseEvent( Name::DeviceSettingUpdated, json ) {}

}  // namespace ScriptEvents
}  // namespace CoreD
}  // namespace HubPlatform
