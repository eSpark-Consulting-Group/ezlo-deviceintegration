#include "zwave/script_events/schedule_entry_lock_week_day_updated.h"
#include "zwave/resources/zwave_resources.h"

#include <znet/znet_week_day.hpp>

#include <cassert>

namespace HubPlatform {
namespace Zwave {

ScheduleEntryLockWeekDayUpdated::ScheduleEntryLockWeekDayUpdated( znet_node_id_t nodeId,
                                                                  znet_node_channel_id_t channelId,
                                                                  const znet_week_day_schedule_t& value )
: ValueEvent( Resources::ScriptEvent::Name::ScheduleEntryLockWeekDayUpdated,
              nodeId,
              ZNET_COMMAND_CLASS_SCHEDULE_ENTRY_LOCK,
              channelId )
, m_value( value ) {}

Json ScheduleEntryLockWeekDayUpdated::toJson() const {
    auto result = ValueEvent::toJson();

    using namespace Resources::ScriptEvent;

    result[ Param::UserId ] = m_value.schedule_id.user_id;
    result[ Param::SlotId ] = m_value.schedule_id.slot_id;

    if( is_week_day_schedule_valid( &m_value ) ) {
        auto& value = result[ Param::Value ];

        auto weekDayName = znet::getWeekDayName( static_cast< znet::WeekDay >( m_value.day_of_week ) );
        assert( weekDayName );

        value[ Param::WeekDay ] = weekDayName;
        value[ Param::StartHour ] = m_value.start_hour;
        value[ Param::StartMin ] = m_value.start_min;
        value[ Param::StopHour ] = m_value.stop_hour;
        value[ Param::StopMin ] = m_value.stop_min;
    }

    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
