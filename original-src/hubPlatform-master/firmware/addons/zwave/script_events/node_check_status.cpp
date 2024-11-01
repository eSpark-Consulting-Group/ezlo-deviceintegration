#include "zwave/script_events/node_check_status.h"

#include "znet/converters/mode_status_converter.hpp"

namespace HubPlatform {
namespace Zwave {

NodeCheckStatus::NodeCheckStatus( uint8_t nodeId, znet::ModeStatus status )
: NodeEvent( Resources::ScriptEvent::Name::NodeCheckStatus, nodeId )
, m_status( status ) {}

Json NodeCheckStatus::toJson() const {
    using namespace Resources::ScriptEvent;

    auto result = NodeEvent::toJson();
    result[ Param::Status ] = znet::mode_status_converter::toString( m_status );
    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
