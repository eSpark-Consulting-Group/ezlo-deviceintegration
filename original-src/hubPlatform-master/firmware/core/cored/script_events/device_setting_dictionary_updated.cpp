#include "cored/script_events/device_setting_dictionary_updated.h"
#include "cored/script_events/resources/script_event_resources.h"

namespace HubPlatform {
namespace CoreD {
namespace ScriptEvents {

DeviceSettingDictionaryUpdated::DeviceSettingDictionaryUpdated( const Json& json )
: BaseEvent( Name::DeviceSettingDictionaryUpdated, json ) {}

}  // namespace ScriptEvents
}  // namespace CoreD
}  // namespace HubPlatform
