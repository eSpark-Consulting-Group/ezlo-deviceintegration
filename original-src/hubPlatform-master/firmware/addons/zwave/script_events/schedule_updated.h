#pragma once

#include "zwave/script_events/value_event.h"

#include <znet/schedule.hpp>

namespace HubPlatform {
namespace Zwave {

class ScheduleUpdated : public ValueEvent {
public:
    ScheduleUpdated( znet_node_id_t nodeId,
                     znet_node_channel_id_t channelId,
                     const uint8_t scheduleId,
                     const znet::ScheduleIdBlockOpt& idBlockOpt,
                     const znet::Schedule& value );

    Json toJson() const override;

private:
    Json generateValueJson() const;

    uint8_t m_scheduleId;
    znet::ScheduleIdBlockOpt m_idBlockOpt;
    znet::Schedule m_schedule;
};

}  // namespace Zwave
}  // namespace HubPlatform
