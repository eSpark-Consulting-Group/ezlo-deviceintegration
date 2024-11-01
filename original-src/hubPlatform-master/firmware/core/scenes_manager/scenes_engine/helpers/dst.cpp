#include "dst.h"
#include "base_utils/gregorian_calendar.h"

namespace SceneEngine {

/** Calculates if the current timezone has Daylight Savings Time change.
 * @returns true is it has DST, false - otherwise. */
bool DaylightSavingsTime::hasDSTChange( IClock& clocks ) {
    using namespace std::chrono;
    auto nowUtc = clocks.greenwichMeanTimePoint();
    auto nowTimeZoneOffset = clocks.timeZoneOffset();
    decltype( nowUtc ) zero;

    auto futureTimePoint = ( nowUtc + seconds( 4 * 30 * 86400 ) );
    auto offsetInFuture = clocks.timeZoneOffset( GregorianCalendar::TimePoint( futureTimePoint ) );

    if( nowTimeZoneOffset != offsetInFuture )
        return true;

    // Add another 4 months
    futureTimePoint = nowUtc + seconds( 8 * 30 * 86400 );
    offsetInFuture = clocks.timeZoneOffset( GregorianCalendar::TimePoint( futureTimePoint ) );

    if( nowTimeZoneOffset != offsetInFuture )
        return true;

    return false;
}

}  // namespace SceneEngine