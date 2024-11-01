// Extracted from date_event.h/cpp
#include "scenes_manager/scenes_engine/events/daily_event.h"
#include "log_utils/ezlo_log.h"

namespace SceneEngine {

DailyEvent::DailyEvent( TimeList timeList )
: m_timeList( std::move( timeList ) ) {}

/** For today and tomorrow, calculates next date event time point using time list: gets current day,
 *      sets time to local time from time list, calls findNextTime to it.
 *  @returns next event time point during current and next day.
 *  @returns nullptr if there are no more events. */
DateEvent::OptionalClock DailyEvent::getNextDate( const GregorianCalendar::TimePoint& localNow,
                                                  TimeZoneOffset offset ) const {
    GregorianCalendar::Date date = GregorianCalendar::Date::fromLocalTime( localNow, offset );
    for( int i = 0; i <= 1; ++i, date.incrementDay() ) {
        if( auto clock = findNextTime( localNow.time_since_epoch(), date, m_timeList ) ) {
            return clock;
        }
    }
    LOGGER_ERROR( "next date not found" );
    return nullptr;
}

}  // namespace SceneEngine