#include "cored/script_events/item_dictionary_updated.h"
#include "cored/script_events/resources/script_event_resources.h"

namespace HubPlatform {
namespace CoreD {
namespace ScriptEvents {

ItemDictionaryUpdated::ItemDictionaryUpdated( const Json& json )
: BaseEvent( Name::ItemDictionaryUpdated, json ) {}

}  // namespace ScriptEvents
}  // namespace CoreD
}  // namespace HubPlatform
