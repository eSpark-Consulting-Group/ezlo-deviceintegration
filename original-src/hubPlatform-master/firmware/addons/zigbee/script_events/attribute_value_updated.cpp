#include "zigbee/script_events/attribute_value_updated.h"
#include "zigbee/model/zigbee_attribute.h"
#include "zigbee/resources/zigbee_resources.h"

namespace HubPlatform {
namespace Zigbee {

AttributeValueUpdated::AttributeValueUpdated( uint16_t nodeId,
                                              uint16_t endpointId,
                                              const std::string& clusterName,
                                              const ZigbeeAttribute& attribute )
: NodeEvent( Resources::ScriptEvent::Name::AttributeValueUpdated, nodeId )
, m_endpointId( endpointId )
, m_clusterName( clusterName )
, m_attribute( attribute ) {}

Json AttributeValueUpdated::toJson() const {
    auto result = NodeEvent::toJson();
    result[ Resources::ScriptEvent::Param::EndpointId ] = m_endpointId;
    result[ Resources::ScriptEvent::Param::ClusterName ] = m_clusterName;
    result[ Resources::ScriptEvent::Param::Attribute ] = formatObject( m_attribute.toJson() );

    return std::move( result );
}

}  // namespace Zigbee
}  // namespace HubPlatform
