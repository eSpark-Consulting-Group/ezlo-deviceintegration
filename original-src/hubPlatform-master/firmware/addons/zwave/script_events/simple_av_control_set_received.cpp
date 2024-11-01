#include "zwave/script_events/simple_av_control_set_received.h"

#include "zwave/resources/zwave_resources.h"

#include <znet/znet_defs.h>

namespace HubPlatform {
namespace Zwave {

SimpleAvControlSetReceived::SimpleAvControlSetReceived( uint8_t nodeId,
                                                        uint8_t channelId,
                                                        uint8_t sequenceNum,
                                                        const std::string& keyAttribute,
                                                        std::vector< uint16_t >&& commands )
: ValueEvent( Resources::ScriptEvent::Name::SimpleAvControlSetReceived,
              nodeId,
              ZNET_COMMAND_CLASS_SIMPLE_AV_CONTROL,
              channelId )
, m_sequenceNum( sequenceNum )
, m_keyAttribute( keyAttribute )
, m_commands( std::move( commands ) ) {}

Json SimpleAvControlSetReceived::toJson() const {
    using namespace Resources::ScriptEvent;

    auto result = ValueEvent::toJson();
    result[ Param::SequenceNumber ] = m_sequenceNum;
    result[ Param::KeyAttribute ] = m_keyAttribute;
    result[ Param::Commands ] = m_commands;
    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
