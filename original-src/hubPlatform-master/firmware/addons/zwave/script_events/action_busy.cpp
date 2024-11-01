#include "zwave/script_events/action_busy.h"

#include "znet/converters/mode_converter.hpp"

namespace HubPlatform {
namespace Zwave {

ActionBusy::ActionBusy( znet::IZNet::Mode mode )
: Services::ScriptEvent( Resources::ScriptEvent::Name::ActionBusy )
, m_mode( mode ) {}

Json ActionBusy::toJson() const {
    using namespace Resources::ScriptEvent;

    auto result = Services::ScriptEvent::toJson();
    result[ Param::ActionType ] = znet::mode_converter::toString( m_mode );
    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
