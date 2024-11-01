#pragma once

#include "zwave/script_events/value_event.h"

#include <znet/enums/user_code.hpp>

namespace HubPlatform {
namespace Zwave {

class UserCodeUpdated : public ValueEvent {
public:
    UserCodeUpdated( uint8_t nodeId,
                     uint8_t channelId,
                     znet::UserCode::UserCodeRanges&& usersCodeRanges,
                     uint16_t usersCount,
                     znet::UserCode::UserPollStatus pollingStatus );

    Json toJson() const final;

private:
    znet::UserCode::UserCodeRanges m_userCodesRanges;
    uint16_t m_usersCount;
    znet::UserCode::UserPollStatus m_userPollingStatus;
};

}  // namespace Zwave
}  // namespace HubPlatform
