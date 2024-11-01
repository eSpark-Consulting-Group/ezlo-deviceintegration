#include "zwave/script_events/node_removed.h"

#include "zwave/resources/zwave_resources.h"

namespace HubPlatform {
namespace Zwave {

NodeRemoved::NodeRemoved( uint8_t nodeId, uint16_t manufacturerId, uint16_t productType, uint16_t productId )
: NodeEvent( Resources::ScriptEvent::Name::NodeRemoved, nodeId )
, m_manufacturerId( manufacturerId )
, m_productType( productType )
, m_productId( productId ) {}

Json NodeRemoved::toJson() const {
    using namespace Resources::ScriptEvent;

    auto result = NodeEvent::toJson();
    result[ Param::ManufacturerId ] = m_manufacturerId;
    result[ Param::ProductType ] = m_productType;
    result[ Param::ProductId ] = m_productId;

    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
