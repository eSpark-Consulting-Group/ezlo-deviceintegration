#pragma once

#include "zigbee/script_events/command_received_base.h"

namespace HubPlatform {
namespace Zigbee {

enum class DoorLockEventSource;
enum class DoorLockOperationEvent;

class DoorLockOperatingEventNotificationReceived : public CommandReceivedBase {
public:
    explicit DoorLockOperatingEventNotificationReceived( uint16_t nodeId,
                                                         uint16_t endpointId,
                                                         DoorLockEventSource eventSource,
                                                         DoorLockOperationEvent event,
                                                         uint16_t userId,
                                                         const std::string& pinCode );

    Json toJson() const override;

private:
    DoorLockEventSource m_eventSource;
    DoorLockOperationEvent m_event;
    uint16_t m_userId;
    std::string m_pinCode;
};

}  // namespace Zigbee
}  // namespace HubPlatform
