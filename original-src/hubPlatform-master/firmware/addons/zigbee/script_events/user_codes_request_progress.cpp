#include "zigbee/script_events/user_codes_request_progress.h"
#include "zigbee/resources/zigbee_resources.h"

namespace HubPlatform {
namespace Zigbee {

UserCodesRequestProgress::UserCodesRequestProgress( uint16_t nodeId, uint16_t endpointId, uint8_t progress )
: NodeEvent( Resources::ScriptEvent::Name::UserCodesRequestProgress, nodeId )
, m_endpointId( endpointId )
, m_progress( progress ) {}

Json UserCodesRequestProgress::toJson() const {
    using namespace Resources::ScriptEvent;

    auto result = NodeEvent::toJson();
    result[ Param::EndpointId ] = m_endpointId;
    result[ Param::Value ] = m_progress;
    return result;
}

}  // namespace Zigbee
}  // namespace HubPlatform
