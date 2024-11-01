#include "base_utils/gregorian_calendar.h"
#include "base_utils/gregorian_calendar_weeks.h"

namespace GregorianCalendar {

/** @brief Evaluates the number of a week in a year for the date specified. Possible values: [1..54].
 *  Week 1 contains any day before and including the first Sunday and can be a part of a week.
 */
uint8_t getWeekOfYear( const Date& date ) {
    // The original formula from strftime, format %W; uses Sunday = 0, weeks counted from 0:
    //      (tp->tm_yday - (tp->tm_wday - 1 + 7) % 7 + 7) / 7
    // Notes about the formula:
    //      (tp->tm_wday - 1 + 7) % 7 == conversion to count the start of the week from Monday, Monday = 0
    //      tm_yday == amount of days passed since January 1, counted from 0

    // Adopted formula to use Sunday = 7, weeks counted from 1
    const int64_t fullDaysFromYearBeginning = date.getDaysFromYearBeginning().count() - 1;
    const int64_t zeroBasedWeekday = static_cast< int64_t >( date.getWeekday() ) - 1;
    int64_t result = ( fullDaysFromYearBeginning - zeroBasedWeekday + DaysInWeek ) / DaysInWeek;

    Date firstDayInYear;
    firstDayInYear.setDate( date.getYear(), 1, 1 );
    if( firstDayInYear.getWeekday() != 1 ) {
        // In the reference formula week 1 is the one starting with the first Monday in a year.
        // In our system, where weeks are counted from 1, such formula would evaluate to 2
        // in case, when a year starts with Monday. Since it doesn't make sense, this conditional increment was added.
        result += 1;
    }

    return result;
}

/** @brief Evaluates the number of a week in a month for the date specified. Possible values" [1..6].
 * Week 1 contains any day before and including the first Sunday.
 */
uint8_t getWeekOfMonth( const Date& date ) {
    return ( DaysInWeek + date.getDay() + ( DaysInWeek - date.getWeekday() - 1 ) ) / DaysInWeek;
}

}  // namespace GregorianCalendar
