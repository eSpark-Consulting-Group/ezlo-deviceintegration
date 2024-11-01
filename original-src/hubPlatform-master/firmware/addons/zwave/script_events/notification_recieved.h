#pragma once

#include "zwave/script_events/value_event.h"
#include <znet/znet_notification.hpp>

namespace HubPlatform {
namespace Zwave {

class NotificationReceived : public ValueEvent {
public:
    NotificationReceived( uint8_t nodeId,
                          uint8_t channelId,
                          uint8_t v1AlarmType,
                          uint8_t v1AlarmLevel,
                          znet::NotificationType notification,
                          znet::NotificationEvent event,
                          std::vector< std::uint8_t > value );

    Json toJson() const final;

private:
    uint8_t m_v1AlarmType;
    uint8_t m_v1AlarmLevel;
    znet::NotificationType m_notification;
    znet::NotificationEvent m_event;
    std::vector< std::uint8_t > m_value;
};

}  // namespace Zwave
}  // namespace HubPlatform
