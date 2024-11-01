#pragma once

#include "zwave/script_events/node_event.h"

namespace HubPlatform {
namespace Zwave {

class NodeRemoved final : public NodeEvent {
public:
    NodeRemoved( uint8_t nodeId, uint16_t manufacturerId, uint16_t productType, uint16_t productId );

    Json toJson() const override;

private:
    uint16_t m_manufacturerId;
    uint16_t m_productType;
    uint16_t m_productId;
};

}  // namespace Zwave
}  // namespace HubPlatform
