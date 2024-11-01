#include "cored/script_events/item_added.h"
#include "cored/script_events/resources/script_event_resources.h"

namespace HubPlatform {
namespace CoreD {
namespace ScriptEvents {

ItemAdded::ItemAdded( const Json& json )
: BaseEvent( Name::ItemAdded, json ) {}

}  // namespace ScriptEvents
}  // namespace CoreD
}  // namespace HubPlatform
