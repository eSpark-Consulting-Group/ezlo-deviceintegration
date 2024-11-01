#include "zwave/script_events/node_firmware_update_status_event.h"

#include "zwave/resources/zwave_resources.h"

namespace HubPlatform {
namespace Zwave {

NodeFirmwareUpdateStatusEvent::NodeFirmwareUpdateStatusEvent( uint8_t nodeId,
                                                              znet::utils::optional< uint8_t > target,
                                                              std::string status,
                                                              znet::utils::optional< uint8_t > progress,
                                                              std::string errorDescription )
: NodeEvent( Resources::ScriptEvent::Name::NodeFirmwareUpdateStatus, nodeId )
, m_target( target )
, m_status( std::move( status ) )
, m_progress( progress )
, m_errorDescription( std::move( errorDescription ) ) {}

Json NodeFirmwareUpdateStatusEvent::toJson() const {
    auto result = NodeEvent::toJson();

    result[ Resources::ScriptEvent::Param::Status ] = m_status;

    if( m_target ) {
        result[ Resources::ScriptEvent::Param::Target ] = *m_target;
    }

    if( m_progress ) {
        result[ Resources::ScriptEvent::Param::Progress ] = *m_progress;
    }

    if( !m_errorDescription.empty() ) {
        result[ Resources::ScriptEvent::Param::Error ] = m_errorDescription;
    }

    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
