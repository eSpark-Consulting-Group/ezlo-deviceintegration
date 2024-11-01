#include "zigbee/script_events/unknown_attribute_value_updated.h"
#include "zigbee/resources/zigbee_resources.h"

namespace HubPlatform {
namespace Zigbee {

UnknownAttributeValueUpdated::UnknownAttributeValueUpdated( uint16_t nodeId,
                                                            uint16_t endpointId,
                                                            uint16_t clusterId,
                                                            const std::string& clusterName,
                                                            uint16_t attributeId,
                                                            const std::string& attributeName,
                                                            const std::string& value )
: NodeEvent( Resources::ScriptEvent::Name::UnknownAttributeValueUpdated, nodeId )
, m_endpointId( endpointId )
, m_clusterId( clusterId )
, m_clusterName( clusterName )
, m_attributeId( attributeId )
, m_attributeName( attributeName )
, m_value( value ) {}

Json UnknownAttributeValueUpdated::toJson() const {
    auto result = NodeEvent::toJson();
    result[ Resources::ScriptEvent::Param::EndpointId ] = m_endpointId;
    result[ Resources::ScriptEvent::Param::ClusterId ] = m_clusterId;
    result[ Resources::ScriptEvent::Param::ClusterName ] = m_clusterName;
    result[ Resources::ScriptEvent::Param::AttributeId ] = m_attributeId;
    result[ Resources::ScriptEvent::Param::AttributeName ] = m_attributeName;
    result[ Resources::ScriptEvent::Param::Value ] = m_value;

    return result;
}

}  // namespace Zigbee
}  // namespace HubPlatform
