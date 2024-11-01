// Extracted from date_event.h/cpp
#include "scenes_manager/scenes_engine/events/weekly_event.h"
#include "log_utils/ezlo_log.h"

namespace SceneEngine {

/** Creates a date event with weekdays and time lists. Even ts should be raised only at days from that weekdays list
 * and at times pointed by time list.*/
WeeklyEvent::WeeklyEvent( TimeList timeList, std::set< Day > weekdays )
: m_timeList( std::move( timeList ) )
, m_weekdayList( std::move( weekdays ) ) {}

/** For the current and next week, iterates weekdays to detect nearest day when the time event should be emitted. */
DateEvent::OptionalClock WeeklyEvent::getNextDate( const GregorianCalendar::TimePoint& localNow,
                                                   TimeZoneOffset offset ) const {
    using namespace GregorianCalendar;
    auto date = Date::fromLocalTime( localNow, offset );
    for( int i = 0; i <= 1; ++i, date.incrementWeek() ) {
        for( const auto& weekday : m_weekdayList ) {
            date.moveToWeekday( weekday );
            if( auto clock = findNextTime( localNow.time_since_epoch(), date, m_timeList ) ) {
                return clock;
            }
        }
    }
    LOGGER_ERROR( "next date not found" );
    return nullptr;
}

}  // namespace SceneEngine