#include "zigbee/script_events/node_removed.h"
#include "zigbee/resources/zigbee_resources.h"

namespace HubPlatform {
namespace Zigbee {

NodeRemoved::NodeRemoved( uint16_t nodeId, std::string manufacturerId, std::string modelId )
: NodeEvent( Resources::ScriptEvent::Name::NodeRemoved, nodeId )
, m_manufacturerId( std::move( manufacturerId ) )
, m_modelId( std::move( modelId ) ) {}

Json NodeRemoved::toJson() const {
    auto result = NodeEvent::toJson();

    result[ Resources::ScriptEvent::Param::ManufacturerId ] = m_manufacturerId;
    result[ Resources::ScriptEvent::Param::ModelId ] = m_modelId;

    return result;
}

}  // namespace Zigbee
}  // namespace HubPlatform
