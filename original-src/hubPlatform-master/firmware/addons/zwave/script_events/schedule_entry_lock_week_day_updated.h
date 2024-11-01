#pragma once

#include "zwave/script_events/value_event.h"

#include <znet/znet.h>

namespace HubPlatform {
namespace Zwave {

class ScheduleEntryLockWeekDayUpdated : public ValueEvent {
public:
    ScheduleEntryLockWeekDayUpdated( znet_node_id_t nodeId,
                                     znet_node_channel_id_t channelId,
                                     const znet_week_day_schedule_t& value );

    Json toJson() const override;

private:
    znet_week_day_schedule_t m_value;
};

}  // namespace Zwave
}  // namespace HubPlatform
