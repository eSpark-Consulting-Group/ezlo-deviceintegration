#pragma once

#include "zigbee/script_events/node_event.h"

namespace HubPlatform {
namespace Zigbee {

class ZigbeeAttribute;

class AttributeValueUpdated : public NodeEvent {
public:
    explicit AttributeValueUpdated( uint16_t nodeId,
                                    uint16_t endpointId,
                                    const std::string& clusterName,
                                    const ZigbeeAttribute& attribute );

    Json toJson() const override;

private:
    uint16_t m_endpointId;
    std::string m_clusterName;
    const ZigbeeAttribute& m_attribute;
};

}  // namespace Zigbee
}  // namespace HubPlatform
