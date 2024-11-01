#pragma once

#include "zigbee/script_events/command_received_base.h"

namespace HubPlatform {
namespace Zigbee {

enum class DoorLockEventSource;
enum class DoorLockProgrammingEvent;
enum class UserType;
enum class UserStatus;

class DoorLockProgrammingEventNotificationReceived : public CommandReceivedBase {
public:
    explicit DoorLockProgrammingEventNotificationReceived( uint16_t nodeId,
                                                           uint16_t endpointId,
                                                           DoorLockEventSource eventSource,
                                                           DoorLockProgrammingEvent event,
                                                           uint16_t userId,
                                                           const std::string& pinCode,
                                                           UserType userType,
                                                           UserStatus userStatus );

    Json toJson() const override;

private:
    DoorLockEventSource m_eventSource;
    DoorLockProgrammingEvent m_event;
    uint16_t m_userId;
    std::string m_pinCode;
    UserType m_userType;
    UserStatus m_userStatus;
};

}  // namespace Zigbee
}  // namespace HubPlatform
