#include "cored/house_modes/script_events/base_event.h"
#include "cored/resources/core_resources.h"

namespace HubPlatform {
namespace CoreDHouseModes {
namespace ScriptEvents {

BaseEvent::BaseEvent( const std::string& event )
: Services::ScriptEvent( event, CoreD::Resources::ModuleNames::HouseModes ) {}

}  // namespace ScriptEvents
}  // namespace CoreDHouseModes
}  // namespace HubPlatform
