#include "scenes_manager/scenes_engine/events/date_event.h"
#include "common_utils/cpp14.h"
#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"
#include "base_utils/gregorian_calendar.h"

namespace SceneEngine {

std::string DateEvent::getMethodName() const {
    return Model::Resources::MethodNames::isDate;
}

/** @brief Iterates time list, sets local time of the current date to the values from the time list.
 * When timestamp of the next time point is in the future comparing to the current local time, returns that time point.
 * @param localNow - local time timestamp WITHOUT respect to timezone offset
 * @param date - local time date, used as a temporary to calculate timestamps of events set by timeList.
 * @param timeList - list of "hours:minutes" time points when event should trigger during the day.
 * @returns local time of the next time event.
 * @returns null - no future time is found;
 */
DateEvent::OptionalClock DateEvent::findNextTime( GregorianCalendar::ClockDuration localNow,
                                                  GregorianCalendar::Date& date,
                                                  const TimeList& timeList ) const {
    for( const auto& time : timeList ) {
        date.setHour( time.first );
        date.setMinute( time.second );
        date.setSecond( 0 );
        date.setMillisecond( 0 );

        auto nextEventTime = date.toClockDuration();
        auto localEventTime = GregorianCalendar::Date::toLocalTime( nextEventTime, date.getTimeZoneOffset() );
        const bool isNextEventTimeLessThanLocalNow = localEventTime > localNow;

        if( isNextEventTimeLessThanLocalNow ) {
            LOGGER_TRACE( "next time is found: " + std::to_string( date.toSeconds().count() ) );
            // Change: returns local time, not UTC!
            return std::make_unique< LocalTimePoint >( localEventTime, date.getTimeZoneOffset(), true );
        }
    }
    LOGGER_TRACE( "next time not found" );
    return nullptr;
}

}  // namespace SceneEngine
