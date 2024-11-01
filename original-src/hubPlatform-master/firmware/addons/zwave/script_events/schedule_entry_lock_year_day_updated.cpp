#include "zwave/script_events/schedule_entry_lock_year_day_updated.h"
#include "zwave/resources/zwave_resources.h"

namespace HubPlatform {
namespace Zwave {

ScheduleEntryLockYearDayUpdated::ScheduleEntryLockYearDayUpdated( znet_node_id_t nodeId,
                                                                  znet_node_channel_id_t channelId,
                                                                  const znet_year_day_schedule_t& value,
                                                                  uint8_t century )
: ValueEvent( Resources::ScriptEvent::Name::ScheduleEntryLockYearDayUpdated,
              nodeId,
              ZNET_COMMAND_CLASS_SCHEDULE_ENTRY_LOCK,
              channelId )
, m_value( value )
, m_century( century ) {}

Json ScheduleEntryLockYearDayUpdated::toJson() const {
    auto result = ValueEvent::toJson();

    using namespace Resources::ScriptEvent;

    result[ Param::UserId ] = m_value.schedule_id.user_id;
    result[ Param::SlotId ] = m_value.schedule_id.slot_id;

    if( is_year_day_schedule_valid( &m_value ) ) {
        auto& value = result[ Param::Value ];

        value[ Param::StartYear ] = m_century * 100 + m_value.start_year;
        value[ Param::StartMonth ] = m_value.start_month;
        value[ Param::StartDay ] = m_value.start_day;
        value[ Param::StartHour ] = m_value.start_hour;
        value[ Param::StartMin ] = m_value.start_minute;

        value[ Param::StopYear ] = m_century * 100 + m_value.stop_year;
        value[ Param::StopMonth ] = m_value.stop_month;
        value[ Param::StopDay ] = m_value.stop_day;
        value[ Param::StopHour ] = m_value.stop_hour;
        value[ Param::StopMin ] = m_value.stop_minute;
    }

    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
