#include "zwave/script_events/value_event.h"

#include "zwave/resources/zwave_resources.h"

namespace HubPlatform {
namespace Zwave {

ValueEvent::ValueEvent( std::string eventName, uint8_t nodeId, uint8_t classId, uint8_t channelId )
: NodeEvent( std::move( eventName ), nodeId )
, m_classId( classId )
, m_channelId( channelId ) {}

Json ValueEvent::toJson() const {
    using namespace Resources::ScriptEvent;

    auto result = NodeEvent::toJson();
    result[ Param::ClassId ] = m_classId;
    result[ Param::ChannelId ] = m_channelId;
    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform