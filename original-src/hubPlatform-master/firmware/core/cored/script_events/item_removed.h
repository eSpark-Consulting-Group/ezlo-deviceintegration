#include "cored/script_events/base_event.h"

namespace HubPlatform {
namespace CoreD {
namespace ScriptEvents {

class ItemRemoved : public BaseEvent {
public:
    ItemRemoved( const Json& json );
};

}  // namespace ScriptEvents
}  // namespace CoreD
}  // namespace HubPlatform
