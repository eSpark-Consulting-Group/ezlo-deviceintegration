#include "cored/script_events/item_removed.h"
#include "cored/script_events/resources/script_event_resources.h"

namespace HubPlatform {
namespace CoreD {
namespace ScriptEvents {

ItemRemoved::ItemRemoved( const Json& json )
: BaseEvent( Name::ItemRemoved, json ) {}

}  // namespace ScriptEvents
}  // namespace CoreD
}  // namespace HubPlatform
