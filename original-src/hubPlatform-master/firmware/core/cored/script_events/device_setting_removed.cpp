#include "cored/script_events/device_setting_removed.h"
#include "cored/script_events/resources/script_event_resources.h"

namespace HubPlatform {
namespace CoreD {
namespace ScriptEvents {

DeviceSettingRemoved::DeviceSettingRemoved( const Json& json )
: BaseEvent( Name::DeviceSettingRemoved, json ) {}

}  // namespace ScriptEvents
}  // namespace CoreD
}  // namespace HubPlatform
