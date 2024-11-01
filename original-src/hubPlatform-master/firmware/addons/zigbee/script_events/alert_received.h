#pragma once

#include "zigbee/model/data_structures/alert_structures.h"
#include "zigbee/script_events/command_received_base.h"

namespace HubPlatform {
namespace Zigbee {

class AlertReceived : public CommandReceivedBase {
public:
    explicit AlertReceived( uint16_t nodeId,
                            uint8_t endpointId,
                            const HubPlatform::Zigbee::AlertStructure& alertStructures,
                            const char* commandName,
                            AlertSource eventNotificationId );

    Json toJson() const override;

private:
    AlertStructure m_alertStructures;
};

}  // namespace Zigbee
}  // namespace HubPlatform
