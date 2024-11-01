#include "zwave/script_events/schedule_entry_lock_daily_repeating_updated.h"
#include "zwave/resources/zwave_resources.h"

#include <znet/znet_week_day.hpp>

namespace HubPlatform {
namespace Zwave {

ScheduleEntryLockDailyRepeatingUpdated::ScheduleEntryLockDailyRepeatingUpdated(
    znet_node_id_t nodeId,
    znet_node_channel_id_t channelId,
    const znet_daily_repeating_schedule_t& value )
: ValueEvent( Resources::ScriptEvent::Name::ScheduleEntryLockDailyRepeatingUpdated,
              nodeId,
              ZNET_COMMAND_CLASS_SCHEDULE_ENTRY_LOCK,
              channelId )
, m_value( value ) {}

Json ScheduleEntryLockDailyRepeatingUpdated::toJson() const {
    auto result = ValueEvent::toJson();

    using namespace Resources::ScriptEvent;

    result[ Param::UserId ] = m_value.schedule_id.user_id;
    result[ Param::SlotId ] = m_value.schedule_id.slot_id;

    if( is_daily_repeating_valid( &m_value ) && m_value.week_day_bitmask ) {
        auto& value = result[ Param::Value ];
        auto& weekDays = value[ Param::WeekDays ] = Json::array();

        znet::utils::forEachBit(
            &m_value.week_day_bitmask, sizeof( m_value.week_day_bitmask ), [&weekDays]( uint8_t weekDayIndex ) {
                auto weekDayName = znet::getWeekDayName( static_cast< znet::WeekDay >( weekDayIndex ) );
                assert( weekDayName );
                weekDays.push_back( weekDayName );
            } );

        value[ Param::StartHour ] = m_value.start_hour;
        value[ Param::StartMin ] = m_value.start_min;
        value[ Param::DurationHour ] = m_value.duration_hour;
        value[ Param::DurationMin ] = m_value.duration_min;
    }

    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
