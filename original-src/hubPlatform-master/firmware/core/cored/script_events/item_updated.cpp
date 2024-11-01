#include "cored/script_events/item_updated.h"
#include "cored/script_events/resources/script_event_resources.h"

namespace HubPlatform {
namespace CoreD {
namespace ScriptEvents {

ItemUpdated::ItemUpdated( const Json& json )
: BaseEvent( Name::ItemUpdated, json ) {}

}  // namespace ScriptEvents
}  // namespace CoreD
}  // namespace HubPlatform
