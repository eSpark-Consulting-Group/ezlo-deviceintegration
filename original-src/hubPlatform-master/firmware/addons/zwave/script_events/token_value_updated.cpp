#include "zwave/script_events/token_value_updated.h"

#include "zwave/resources/zwave_resources.h"

namespace HubPlatform {
namespace Zwave {

TokenValueUpdated::TokenValueUpdated( uint8_t nodeId, uint8_t classId, uint8_t channelId, const std::string& value )
: ValueEvent( Resources::ScriptEvent::Name::TokenValueUpdated, nodeId, classId, channelId )
, m_value( value ) {}

Json TokenValueUpdated::toJson() const {
    using namespace Resources::ScriptEvent;

    auto result = ValueEvent::toJson();
    result[ Param::Value ] = m_value;
    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
