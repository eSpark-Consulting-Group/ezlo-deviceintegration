#include "zwave/script_events/schedule_updated.h"
#include "zwave/resources/zwave_resources.h"
#include <znet/converters/schedule_converter.hpp>

namespace HubPlatform {
namespace Zwave {

ScheduleUpdated::ScheduleUpdated( znet_node_id_t nodeId,
                                  znet_node_channel_id_t channelId,
                                  uint8_t scheduleId,
                                  const znet::ScheduleIdBlockOpt& idBlockOpt,
                                  const znet::Schedule& value )
: ValueEvent( Resources::ScriptEvent::Name::ScheduleUpdated, nodeId, ZNET_COMMAND_CLASS_SCHEDULE, channelId )
, m_schedule( value )
, m_scheduleId( scheduleId )
, m_idBlockOpt( idBlockOpt ) {}

Json ScheduleUpdated::toJson() const {
    auto result = ValueEvent::toJson();

    using namespace Resources::ScriptEvent::Param::Schedule;

    result[ ScheduleId ] = m_scheduleId;

    if( m_idBlockOpt ) {
        result[ ScheduleIdBlock ] = ( *m_idBlockOpt );
    }

    result[ Resources::ScriptEvent::Param::Value ] = generateValueJson();
    return result;
}

Json ScheduleUpdated::generateValueJson() const {
    auto fieldValue = Json::object();

    using namespace Resources::ScriptEvent::Param::Schedule;

    auto& yearday = fieldValue[ Yearday ] = Json::object();

    if( m_schedule.yearday.startYearOpt ) {
        yearday[ StartYear ] = ( *m_schedule.yearday.startYearOpt );
    }

    if( m_schedule.yearday.startMonthOpt ) {
        yearday[ StartMonth ] = ( *m_schedule.yearday.startMonthOpt );
    }

    yearday[ StartDayOfMonth ] = m_schedule.yearday.startDayOfMonth;

    if( m_schedule.weekdays.size() > 0 ) {
        auto& weekdays = fieldValue[ Weekdays ] = Json::array();
        for( const auto& weekday : m_schedule.weekdays ) {
            if( weekday == znet::ScheduleWeeksDay::Undefined ) {
                continue;
            }
            weekdays.push_back( znet::schedule_weeks_day_converter::weeksDayToString( weekday ) );
        }
    }

    auto& hourminute = fieldValue[ Hourandminute ] = Json::object();

    if( m_schedule.hourminute.startHourOpt ) {
        hourminute[ StartHour ] = ( *m_schedule.hourminute.startHourOpt );
    }

    if( m_schedule.hourminute.startMinuteOpt ) {
        hourminute[ StartMinute ] = ( *m_schedule.hourminute.startMinuteOpt );
    }

    auto& options = fieldValue[ Options ] = Json::object();

    if( m_schedule.options.activityStatusOpt ) {
        options[ ActivityStatus ] =
            znet::schedule_active_status_converter::activeStatusToString( *m_schedule.options.activityStatusOpt );
    }

    options[ DurationType ] =
        znet::schedule_duration_type_converter::durationTypeToString( m_schedule.options.durationType );

    if( m_schedule.options.duration ) {
        options[ Duration ] = ( *m_schedule.options.duration );
    }

    if( m_schedule.options.recurrenceOffsetOpt ) {
        options[ RecurrenceOffset ] = ( *m_schedule.options.recurrenceOffsetOpt );
    }

    if( m_schedule.options.recurrenceModeOpt ) {
        options[ RecurrenceMode ] =
            znet::schedule_recurrence_mode_converter::modeToString( *m_schedule.options.recurrenceModeOpt );
    }

    if( m_schedule.options.relativeOpt ) {
        options[ Relative ] = ( *m_schedule.options.relativeOpt );
    }

    if( m_schedule.options.reportToFollowOpt ) {
        options[ ReportToFollow ] = ( *m_schedule.options.reportToFollowOpt );
    }

    auto& commands = fieldValue[ Commands ] = Json::array();
    for( const auto& command : m_schedule.commands ) {
        auto commandsBytes = Json::array();
        for( const auto& cmdByte : command ) {
            commandsBytes.push_back( cmdByte );
        }
        commands.push_back( commandsBytes );
    }

    return fieldValue;
}

}  // namespace Zwave
}  // namespace HubPlatform
