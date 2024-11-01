#include "cored/script_events/gateway_setting_updated.h"
#include "cored/script_events/resources/script_event_resources.h"

namespace HubPlatform {
namespace CoreD {
namespace ScriptEvents {

GatewaySettingUpdated::GatewaySettingUpdated( const Json& json )
: BaseEvent( Name::GatewaySettingUpdated, json ) {}

}  // namespace ScriptEvents
}  // namespace CoreD
}  // namespace HubPlatform
