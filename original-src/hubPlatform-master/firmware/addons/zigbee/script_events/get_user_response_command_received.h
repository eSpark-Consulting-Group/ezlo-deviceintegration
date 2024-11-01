#pragma once

#include "zigbee/script_events/command_received_base.h"

namespace HubPlatform {
namespace Zigbee {

enum class UserStatus;
enum class UserType;

class GetUserResponseReceived : public CommandReceivedBase {
public:
    explicit GetUserResponseReceived( uint16_t nodeId,
                                      uint16_t endpointId,
                                      uint16_t userId,
                                      UserStatus userStatus,
                                      UserType userType,
                                      const std::string& pinCode );

    Json toJson() const override;

private:
    uint16_t m_userId;
    UserStatus m_userStatus;
    UserType m_userType;
    std::string m_pinCode;
};

}  // namespace Zigbee
}  // namespace HubPlatform
