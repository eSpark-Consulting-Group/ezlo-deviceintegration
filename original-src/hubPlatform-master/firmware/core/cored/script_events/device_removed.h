#pragma once

#include "cored/script_events/base_event.h"

namespace HubPlatform {
namespace CoreD {
namespace ScriptEvents {

class DeviceRemoved : public BaseEvent {
public:
    DeviceRemoved( const Json& json );
};

}  // namespace ScriptEvents
}  // namespace CoreD
}  // namespace HubPlatform
