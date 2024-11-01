#pragma once

#include "cored/script_events/base_event.h"

namespace HubPlatform {
namespace CoreD {
namespace ScriptEvents {

class GatewaySettingUpdated final : public BaseEvent {
public:
    GatewaySettingUpdated( const Json& json );
};

}  // namespace ScriptEvents
}  // namespace CoreD
}  // namespace HubPlatform
