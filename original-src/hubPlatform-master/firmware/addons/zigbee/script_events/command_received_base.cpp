#include "zigbee/script_events/command_received_base.h"
#include "zigbee/model/enums/command_type.h"
#include "zigbee/resources/zigbee_resources.h"

namespace HubPlatform {
namespace Zigbee {

CommandReceivedBase::CommandReceivedBase( std::string eventName,
                                          uint16_t nodeId,
                                          uint16_t endpointId,
                                          const std::string& clusterName,
                                          uint8_t commandId,
                                          const std::string& commandName )
: NodeEvent( eventName, nodeId )
, m_endpointId( endpointId )
, m_clusterName( clusterName )
, m_commandId( commandId )
, m_commandName( commandName ) {}

Json CommandReceivedBase::toJson() const {
    auto result = NodeEvent::toJson();
    result[ Resources::ScriptEvent::Param::EndpointId ] = m_endpointId;
    result[ Resources::ScriptEvent::Param::ClusterName ] = m_clusterName;
    result[ Resources::ScriptEvent::Param::CommandId ] = m_commandId;
    result[ Resources::ScriptEvent::Param::CommandName ] = m_commandName;

    return std::move( result );
}

}  // namespace Zigbee
}  // namespace HubPlatform
