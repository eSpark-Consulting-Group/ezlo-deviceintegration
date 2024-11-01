#include "cored/script_events/gateway_setting_added.h"
#include "cored/script_events/resources/script_event_resources.h"

namespace HubPlatform {
namespace CoreD {
namespace ScriptEvents {

GatewaySettingAdded::GatewaySettingAdded( const Json& json )
: BaseEvent( Name::GatewaySettingAdded, json ) {}

}  // namespace ScriptEvents
}  // namespace CoreD
}  // namespace HubPlatform
