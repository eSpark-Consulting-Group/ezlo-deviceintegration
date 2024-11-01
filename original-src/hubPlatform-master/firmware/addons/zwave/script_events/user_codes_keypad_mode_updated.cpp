#include "zwave/script_events/user_codes_keypad_mode_updated.h"
#include "zwave/resources/zwave_resources.h"

#include <znet/znet_defs.h>

namespace HubPlatform {
namespace Zwave {

using namespace Resources::ScriptEvent;

UserCodesKeypadModeUpdated::UserCodesKeypadModeUpdated( uint8_t nodeId, uint8_t channelId, std::string mode )
: ValueEvent( Name::UserCodesKeypadModeUpdated, nodeId, ZNET_COMMAND_CLASS_USER_CODE, channelId )
, m_mode( std::move( mode ) ) {}

Json UserCodesKeypadModeUpdated::toJson() const {
    auto result = ValueEvent::toJson();
    result[ Param::Value ] = m_mode;

    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
