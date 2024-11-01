#include "zwave/script_events/button_state_updated.h"

#include "zwave/resources/zwave_resources.h"

#include <znet/znet_defs.h>

namespace HubPlatform {
namespace Zwave {

ButtonStateUpdated::ButtonStateUpdated( uint8_t nodeId, uint8_t channelId, uint8_t buttonNum, const std::string& value )
: ValueEvent( Resources::ScriptEvent::Name::ButtonStateUpdated, nodeId, ZNET_COMMAND_CLASS_CENTRAL_SCENE, channelId )
, m_buttonNum( buttonNum )
, m_value( value ) {}

Json ButtonStateUpdated::toJson() const {
    using namespace Resources::ScriptEvent;

    auto result = ValueEvent::toJson();
    result[ Param::ButtonId ] = m_buttonNum;
    result[ Param::Value ] = m_value;
    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
