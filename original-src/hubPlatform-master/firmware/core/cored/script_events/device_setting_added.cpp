#include "cored/script_events/device_setting_added.h"
#include "cored/script_events/resources/script_event_resources.h"

namespace HubPlatform {
namespace CoreD {
namespace ScriptEvents {

DeviceSettingAdded::DeviceSettingAdded( const Json& json )
: BaseEvent( Name::DeviceSettingAdded, json ) {}

}  // namespace ScriptEvents
}  // namespace CoreD
}  // namespace HubPlatform
