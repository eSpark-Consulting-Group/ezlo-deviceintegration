// Extracted from date_event.h/cpp
#include "scenes_manager/scenes_engine/events/monthly_event.h"
#include "log_utils/ezlo_log.h"

namespace SceneEngine {

MonthlyEvent::MonthlyEvent( TimeList timeList, std::set< Day > days )
: m_timeList( std::move( timeList ) )
, m_dayList( std::move( days ) ) {}

/** For current and next months, calculates next date event time point using time and day lists: gets current day,
 *      sets time to local time from time list, calls findNextTime to it.
 *  @returns next event time point during current or next month, or null if there is no more events. */
DateEvent::OptionalClock MonthlyEvent::getNextDate( const GregorianCalendar::TimePoint& localNow,
                                                    TimeZoneOffset offset ) const {
    GregorianCalendar::Date date = GregorianCalendar::Date::fromLocalTime( localNow, offset );

    for( int i = 0; i <= 1; ++i, date.incrementMonth() ) {
        for( const auto& day : m_dayList ) {
            date.setDay( day );
            if( auto clock = findNextTime( localNow.time_since_epoch(), date, m_timeList ) ) {
                return clock;
            }
        }
        date.setDay( 1 );
    }
    LOGGER_ERROR( "next date not found" );
    return nullptr;
}

}  // namespace SceneEngine