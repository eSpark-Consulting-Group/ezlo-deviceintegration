#pragma once

#include "zwave/script_events/value_event.h"

#include <znet/znet.h>

namespace HubPlatform {
namespace Zwave {

class ScheduleEntryLockYearDayUpdated : public ValueEvent {
public:
    ScheduleEntryLockYearDayUpdated( znet_node_id_t nodeId,
                                     znet_node_channel_id_t channelId,
                                     const znet_year_day_schedule_t& value,
                                     uint8_t century );

    Json toJson() const override;

private:
    znet_year_day_schedule_t m_value;
    uint8_t m_century;
};

}  // namespace Zwave
}  // namespace HubPlatform
