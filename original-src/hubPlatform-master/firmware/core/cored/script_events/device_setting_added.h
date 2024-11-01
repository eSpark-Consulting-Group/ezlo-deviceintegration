#pragma once

#include "cored/script_events/base_event.h"

namespace HubPlatform {
namespace CoreD {
namespace ScriptEvents {

class DeviceSettingAdded : public BaseEvent {
public:
    DeviceSettingAdded( const Json& json );
};

}  // namespace ScriptEvents
}  // namespace CoreD
}  // namespace HubPlatform
