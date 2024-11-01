#pragma once

#include "zwave/script_events/value_event.h"

#include <znet/schedule.hpp>

namespace HubPlatform {
namespace Zwave {

class ScheduleStateUpdated : public ValueEvent {
public:
    ScheduleStateUpdated( znet_node_id_t nodeId,
                          znet_node_channel_id_t channelId,
                          const znet_cmd_schedule_state_report_t& value,
                          const znet::ScheduleIdBlockOpt& idBlockOpt );

    Json toJson() const override;

private:
    uint8_t m_numberSupportedScheduleIds;
    uint8_t m_reportsToFollow;
    bool m_overrideSupport;
    znet::ScheduleIdBlockOpt m_idBlockOpt;
    std::vector< std::string > m_activityStatuses;
};

}  // namespace Zwave
}  // namespace HubPlatform
