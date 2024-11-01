#include "cored/script_events/gateway_setting_removed.h"
#include "cored/script_events/resources/script_event_resources.h"

namespace HubPlatform {
namespace CoreD {
namespace ScriptEvents {

GatewaySettingRemoved::GatewaySettingRemoved( const Json& json )
: BaseEvent( Name::GatewaySettingRemoved, json ) {}

}  // namespace ScriptEvents
}  // namespace CoreD
}  // namespace HubPlatform
