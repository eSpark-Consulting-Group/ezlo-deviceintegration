#pragma once

#include "zigbee/script_events/node_event.h"

namespace HubPlatform {
namespace Zigbee {

class ZigbeeAttribute;

class MfgSpecificAttributeValueUpdated : public NodeEvent {
public:
    explicit MfgSpecificAttributeValueUpdated( uint16_t nodeId,
                                               uint16_t endpointId,
                                               uint16_t clusterId,
                                               const ZigbeeAttribute& attribute );

    Json toJson() const override;

private:
    uint16_t m_endpointId;
    uint16_t m_clusterId;
    const ZigbeeAttribute& m_attribute;
};

}  // namespace Zigbee
}  // namespace HubPlatform
