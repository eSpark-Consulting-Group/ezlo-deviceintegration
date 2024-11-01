#include "zigbee/script_events/mfg_specific_attribute_value_updated.h"
#include "zigbee/model/zigbee_attribute.h"
#include "zigbee/resources/zigbee_resources.h"

namespace HubPlatform {
namespace Zigbee {

MfgSpecificAttributeValueUpdated::MfgSpecificAttributeValueUpdated( uint16_t nodeId,
                                                                    uint16_t endpointId,
                                                                    uint16_t clusterId,
                                                                    const ZigbeeAttribute& attribute )
: NodeEvent( Resources::ScriptEvent::Name::MfgSpecificAttributeValueUpdated, nodeId )
, m_endpointId( endpointId )
, m_clusterId( clusterId )
, m_attribute( attribute ) {}

Json MfgSpecificAttributeValueUpdated::toJson() const {
    auto result = NodeEvent::toJson();
    result[ Resources::ScriptEvent::Param::EndpointId ] = m_endpointId;
    result[ Resources::ScriptEvent::Param::ClusterId ] = m_clusterId;
    result[ Resources::ScriptEvent::Param::Attribute ] = formatObject( m_attribute.toJson() );

    return std::move( result );
}

}  // namespace Zigbee
}  // namespace HubPlatform
