#pragma once

#include "zigbee/script_events/command_received_base.h"
#include "zigbee/model/data_structures/alert_structures.h"

namespace HubPlatform {
namespace Zigbee {

class EmptyAlertsListResponseReceived : public CommandReceivedBase {
public:
    explicit EmptyAlertsListResponseReceived( uint16_t nodeId, uint8_t endpointId );
};

}  // namespace Zigbee
}  // namespace HubPlatform
