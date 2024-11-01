#include "zwave/script_events/configuration_request_status_updated.h"

#include "zwave/resources/zwave_resources.h"

#include <znet/znet_defs.h>

namespace HubPlatform {
namespace Zwave {

ConfigurationRequestStatusUpdated::ConfigurationRequestStatusUpdated( uint8_t nodeId,
                                                                      uint16_t parameterNumber,
                                                                      znet::RequestStatus::Enum status )
: NodeEvent( Resources::ScriptEvent::Name::ConfigurationRequestStatusUpdated, nodeId )
, m_parameterNumber( parameterNumber )
, m_status( status ) {}

Json ConfigurationRequestStatusUpdated::toJson() const {
    using namespace Resources::ScriptEvent;

    auto result = NodeEvent::toJson();

    result[ Param::ParameterNum ] = m_parameterNumber;
    result[ Param::Status ] = znet::RequestStatus::toString( m_status );

    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
