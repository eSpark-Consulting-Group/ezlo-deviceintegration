#pragma once

#include "cored/script_events/base_event.h"

namespace HubPlatform {
namespace CoreD {
namespace ScriptEvents {

class DeviceSettingRemoved : public BaseEvent {
public:
    DeviceSettingRemoved( const Json& json );
};

}  // namespace ScriptEvents
}  // namespace CoreD
}  // namespace HubPlatform
