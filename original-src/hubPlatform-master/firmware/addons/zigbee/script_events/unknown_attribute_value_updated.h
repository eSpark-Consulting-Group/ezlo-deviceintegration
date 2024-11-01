#pragma once

#include "zigbee/script_events/node_event.h"

namespace HubPlatform {
namespace Zigbee {

class ZigbeeAttribute;

class UnknownAttributeValueUpdated : public NodeEvent {
public:
    explicit UnknownAttributeValueUpdated( uint16_t nodeId,
                                           uint16_t endpointId,
                                           uint16_t clusterId,
                                           const std::string& clusterName,
                                           uint16_t attributeId,
                                           const std::string& attributeName,
                                           const std::string& value );

    Json toJson() const override;

private:
    uint16_t m_endpointId;
    uint16_t m_clusterId;
    std::string m_clusterName;
    uint16_t m_attributeId;
    std::string m_attributeName;
    std::string m_value;
};

}  // namespace Zigbee
}  // namespace HubPlatform
