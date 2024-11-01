#pragma once

#include "zigbee/script_events/node_event.h"

namespace HubPlatform {
namespace Zigbee {

class UserCodesRequestProgress : public NodeEvent {
public:
    explicit UserCodesRequestProgress( uint16_t nodeId, uint16_t endpointId, uint8_t progress );

    Json toJson() const override;

private:
    uint16_t m_endpointId;
    uint8_t m_progress;
};

}  // namespace Zigbee
}  // namespace HubPlatform
