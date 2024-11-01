#pragma once

#include "zigbee/script_events/node_event.h"

namespace HubPlatform {
namespace Zigbee {

class NodeRemoved : public NodeEvent {
public:
    explicit NodeRemoved( uint16_t nodeId, std::string manufacturerId, std::string modelId );

    Json toJson() const final;

private:
    std::string m_manufacturerId;
    std::string m_modelId;
};

}  // namespace Zigbee
}  // namespace HubPlatform
