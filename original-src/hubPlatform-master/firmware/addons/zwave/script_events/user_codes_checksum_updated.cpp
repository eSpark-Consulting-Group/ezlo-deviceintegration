#include "zwave/script_events/user_codes_checksum_updated.h"
#include "zwave/resources/zwave_resources.h"

#include <znet/znet_defs.h>

namespace HubPlatform {
namespace Zwave {

using namespace Resources::ScriptEvent;

UserCodesChecksumUpdated::UserCodesChecksumUpdated( uint8_t nodeId, uint8_t channelId, uint16_t checksum )
: ValueEvent( Name::UserCodesChecksumUpdated, nodeId, ZNET_COMMAND_CLASS_USER_CODE, channelId )
, m_checksum( checksum ) {}

Json UserCodesChecksumUpdated::toJson() const {
    auto result = ValueEvent::toJson();
    result[ Param::Value ] = m_checksum;

    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
