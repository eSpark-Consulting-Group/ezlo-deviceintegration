#pragma once

#include "cored/script_events/base_event.h"

namespace HubPlatform {
namespace CoreD {
namespace ScriptEvents {

class DeviceAdded : public BaseEvent {
public:
    DeviceAdded( const Json& json );
};

}  // namespace ScriptEvents
}  // namespace CoreD
}  // namespace HubPlatform
