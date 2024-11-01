#include "zwave/script_events/master_code_updated.h"
#include "zwave/resources/zwave_resources.h"

#include <znet/znet_defs.h>

namespace HubPlatform {
namespace Zwave {

using namespace Resources::ScriptEvent;

MasterCodeUpdated::MasterCodeUpdated( uint8_t nodeId, uint8_t channelId, std::string masterCode )
: ValueEvent( Name::MasterCodeUpdated, nodeId, ZNET_COMMAND_CLASS_USER_CODE, channelId )
, m_masterCode( std::move( masterCode ) ) {}

Json MasterCodeUpdated::toJson() const {
    auto result = ValueEvent::toJson();
    result[ Param::Value ] = m_masterCode;

    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
