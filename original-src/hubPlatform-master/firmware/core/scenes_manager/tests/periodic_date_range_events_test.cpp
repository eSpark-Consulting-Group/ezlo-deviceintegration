#include "scenes_manager/scenes_engine/events/weeks_of_period_event.h"
#include "scenes_manager/scenes_engine/events/days_of_period_event.h"
#include "scenes_manager/scenes_engine/exceptions.h"
#include "json_utils/json_wrapper.h"

#include <gtest/gtest.h>

namespace SceneEngine {
namespace UT {

/*

 TEST PLAN

DONE    1. Weeks of month
DONE        1.1 single week
DONE        1.2 two weeks consecutively
DONE        1.3 last week of month
DONE    2. Weeks of year
DONE        2.1 single week
DONE        2.2 two weeks consecutively
DONE        2.3 last week of year
DONE        2.4 last week of month doesn't activate last week of year range (validation of proper impl)
DONE    3. Days of week
DONE        3.1 single day
DONE        3.2 two days consecutively
DONE        3.3 separate days
DONE        3.4 creating event with invalid weekday number
DONE    4. Days of month
DONE        4.1 single day
DONE        4.2 three days consecutively
DONE        4.3 separated days
DONE        4.4 creating event with invalid day number
DONE    5. Next Execution Date
DONE        5.1 WeeksOfPeriodEvent: Month
DONE        5.2 WeeksOfPeriodEvent: Year
DONE        5.3 DaysOfPeriodEvent: Month
DONE        5.4 DaysOfPeriodEvent: Week

 */

class PeriodicDateRangeEventsTests : public testing::Test {
public:
    using Date = GregorianCalendar::Date;

    static void checkDaysInRange( DateRangeEventBase& event, Date& date, int count );
    static void checkDaysNotInRange( DateRangeEventBase& event, Date& date, int count );
    static void checkPreviousDaysInRange( DateRangeEventBase& event, Date& date, int count );

    static int calcDaysCountToNextWeek( const Date& date );
    static int calcDaysCountToNextMonth( const Date& date );

    static std::string dumpDate( const Date& date );
};

void PeriodicDateRangeEventsTests::checkDaysInRange( DateRangeEventBase& event, Date& date, int count ) {
    for( int i = 0; i < count; ++i ) {
        const bool rangeEdgeCrossed = event.update( date.toTimePoint(), date.getTimeZoneOffset(), true );
        EXPECT_TRUE( i == 0 ? rangeEdgeCrossed : !rangeEdgeCrossed ) << dumpDate( date );
        EXPECT_TRUE( event.isInRange() ) << dumpDate( date );
        date.incrementDay();
    }

    EXPECT_TRUE( event.update( date.toTimePoint(), date.getTimeZoneOffset(), true ) );
    EXPECT_FALSE( event.isInRange() );
}

void PeriodicDateRangeEventsTests::checkDaysNotInRange( DateRangeEventBase& event, Date& date, int count ) {
    for( int i = 0; i < count; ++i ) {
        EXPECT_FALSE( event.update( date.toTimePoint(), date.getTimeZoneOffset(), true ) ) << dumpDate( date );
        EXPECT_FALSE( event.isInRange() ) << dumpDate( date );
        date.incrementDay();
    }
}

void PeriodicDateRangeEventsTests::checkPreviousDaysInRange( DateRangeEventBase& event, Date& date, int count ) {
    EXPECT_FALSE( event.update( date.toTimePoint(), date.getTimeZoneOffset(), true ) );
    EXPECT_FALSE( event.isInRange() );

    for( int i = count; i > 0; --i ) {
        date.decrementDay();
        const bool rangeEdgeCrossed = event.update( date.toTimePoint(), date.getTimeZoneOffset(), true );
        EXPECT_TRUE( i == count ? rangeEdgeCrossed : !rangeEdgeCrossed );
        EXPECT_TRUE( event.isInRange() );
    }

    date.decrementDay();
    EXPECT_TRUE( event.update( date.toTimePoint(), date.getTimeZoneOffset(), true ) );
    EXPECT_FALSE( event.isInRange() );
}

int PeriodicDateRangeEventsTests::calcDaysCountToNextWeek( const Date& date ) {
    return static_cast< int >( GregorianCalendar::DaysInWeek ) - static_cast< int >( date.getWeekday() ) + 1;
}

int PeriodicDateRangeEventsTests::calcDaysCountToNextMonth( const Date& date ) {
    const uint8_t daysInMonth = Date::getDaysInMonth( date.getMonth() );
    return static_cast< int >( daysInMonth ) - static_cast< int >( date.getDay() ) + 1;
}

std::string PeriodicDateRangeEventsTests::dumpDate( const Date& date ) {
    return date.toString( "%Ft%T%z, %A (wday: %u)" );
}

TEST_F( PeriodicDateRangeEventsTests, singleWeekOfMonth ) {  // 1.1
    WeeksOfPeriodEvent event( WeeksOfPeriodEvent::Period::Month, JSON_ARRAY_( 2 ) );
    Date date( "2020-01-01T00:00:00+0000" );
    for( int i = 0; i < 12; ++i ) {
        checkDaysNotInRange( event, date, calcDaysCountToNextWeek( date ) );
        checkDaysInRange( event, date, GregorianCalendar::DaysInWeek );
        checkDaysNotInRange( event, date, calcDaysCountToNextMonth( date ) );
    }
    EXPECT_EQ( date, Date( "2021-01-01T00:00:00+0000" ) );
}

TEST_F( PeriodicDateRangeEventsTests, twoConsecutiveWeeksOfMonth ) {  // 1.2
    WeeksOfPeriodEvent event( WeeksOfPeriodEvent::Period::Month, JSON_ARRAY_( 2, 3 ) );
    Date date( "2020-01-01T00:00:00+0000" );
    for( int i = 0; i < 12; ++i ) {
        checkDaysNotInRange( event, date, calcDaysCountToNextWeek( date ) );
        checkDaysInRange( event, date, GregorianCalendar::DaysInWeek * 2 );
        checkDaysNotInRange( event, date, calcDaysCountToNextMonth( date ) );
    }
    EXPECT_EQ( date, Date( "2021-01-01T00:00:00+0000" ) );
}

TEST_F( PeriodicDateRangeEventsTests, lastWeekOfMonth ) {  // 1.3
    WeeksOfPeriodEvent event( WeeksOfPeriodEvent::Period::Month, JSON_ARRAY_( -1 ) );
    const Date startDate( "2020-01-01T00:00:00+0000" );
    for( uint8_t i = 1; i <= 12; ++i ) {
        Date date = startDate;
        date.setMonth( i );
        date.setDay( 1 );

        int weekday = static_cast< int >( date.getWeekday() );
        int prevMonthLastWeekDays = weekday == 1 ? static_cast< int >( GregorianCalendar::DaysInWeek ) : weekday - 1;
        checkPreviousDaysInRange( event, date, prevMonthLastWeekDays );
    }
}

TEST_F( PeriodicDateRangeEventsTests, singleWeekOfYear ) {  // 2.1
    WeeksOfPeriodEvent event( WeeksOfPeriodEvent::Period::Year, JSON_ARRAY_( 3 ) );
    Date date( "2020-01-01T00:00:00+0000" );
    checkDaysNotInRange( event, date, calcDaysCountToNextWeek( date ) + GregorianCalendar::DaysInWeek );
    checkDaysInRange( event, date, GregorianCalendar::DaysInWeek );
}

TEST_F( PeriodicDateRangeEventsTests, twoConsecutiveWeeksOfYear ) {  // 2.2
    WeeksOfPeriodEvent event( WeeksOfPeriodEvent::Period::Year, JSON_ARRAY_( 3, 4 ) );
    Date date( "2020-01-01T00:00:00+0000" );
    checkDaysNotInRange( event, date, calcDaysCountToNextWeek( date ) + GregorianCalendar::DaysInWeek );
    checkDaysInRange( event, date, GregorianCalendar::DaysInWeek * 2 );
}

TEST_F( PeriodicDateRangeEventsTests, lastWeekOfYear ) {  // 2.3
    WeeksOfPeriodEvent event( WeeksOfPeriodEvent::Period::Year, JSON_ARRAY_( -1 ) );
    Date date( "2020-01-01T00:00:00+0000" );
    ASSERT_EQ( date.getWeekday(), 3 );

    checkPreviousDaysInRange( event, date, static_cast< int >( date.getWeekday() ) - 1 );
}

TEST_F( PeriodicDateRangeEventsTests, lastWeekOfMonthDoestActivateLastWeekOfYear ) {  // 2.4
    WeeksOfPeriodEvent event( WeeksOfPeriodEvent::Period::Year, JSON_ARRAY_( -1 ) );
    Date date( "2020-02-01T00:00:00+0000" );
    ASSERT_EQ( date.getWeekday(), 6 );

    EXPECT_FALSE( event.update( date.toTimePoint(), date.getTimeZoneOffset(), true ) );
    EXPECT_FALSE( event.isInRange() );

    date.decrementDay();
    EXPECT_FALSE( event.update( date.toTimePoint(), date.getTimeZoneOffset(), true ) );
    EXPECT_FALSE( event.isInRange() );
}

TEST_F( PeriodicDateRangeEventsTests, singleDayOfWeek ) {  // 3.1
    const int day = 3;
    DaysOfPeriodEvent event( DaysOfPeriodEvent::Period::Week, JSON_ARRAY_( day ) );
    Date date( "2020-01-01T00:00:00+0000" );
    date.moveToWeekday( 1 );

    checkDaysNotInRange( event, date, day - 1 );
    checkDaysInRange( event, date, 1 );
    checkDaysNotInRange( event, date, GregorianCalendar::DaysInWeek - day );
}

TEST_F( PeriodicDateRangeEventsTests, twoConsecutiveDaysOfWeek ) {  // 3.2
    DaysOfPeriodEvent event( DaysOfPeriodEvent::Period::Week, JSON_ARRAY_( 2, 3 ) );
    Date date( "2020-01-01T00:00:00+0000" );
    date.moveToWeekday( 1 );

    checkDaysNotInRange( event, date, 1 );
    checkDaysInRange( event, date, 2 );
    checkDaysNotInRange( event, date, GregorianCalendar::DaysInWeek - 3 );
}

TEST_F( PeriodicDateRangeEventsTests, twoSeparateDaysOfWeek ) {  // 3.3
    const int day1 = 2;
    const int day2 = 5;
    DaysOfPeriodEvent event( DaysOfPeriodEvent::Period::Week, JSON_ARRAY_( day1, day2 ) );
    Date date( "2020-01-01T00:00:00+0000" );
    date.moveToWeekday( 1 );

    checkDaysNotInRange( event, date, day1 - 1 );
    checkDaysInRange( event, date, 1 );
    checkDaysNotInRange( event, date, day2 - day1 - 1 );
    checkDaysInRange( event, date, 1 );
    checkDaysNotInRange( event, date, GregorianCalendar::DaysInWeek - day2 );
}

TEST_F( PeriodicDateRangeEventsTests, invalidDayOfWeekNumber ) {  // 3.4
    auto period = DaysOfPeriodEvent::Period::Week;
    EXPECT_THROW( DaysOfPeriodEvent( period, JSON_ARRAY_( 10 ) ), SceneException );
}

TEST_F( PeriodicDateRangeEventsTests, singleDayOfMonth ) {  // 4.1
    const int day = 3;
    DaysOfPeriodEvent event( DaysOfPeriodEvent::Period::Month, JSON_ARRAY_( day ) );
    Date date( "2020-01-01T00:00:00+0000" );

    checkDaysNotInRange( event, date, day - 1 );
    checkDaysInRange( event, date, 1 );
    checkDaysNotInRange( event, date, Date::getDaysInMonth( 1 ) - day );

    EXPECT_EQ( date, Date( "2020-02-01T00:00:00+0000" ) );
}

TEST_F( PeriodicDateRangeEventsTests, threeConsecutiveDaysOfMonth ) {  // 4.2
    DaysOfPeriodEvent event( DaysOfPeriodEvent::Period::Month, JSON_ARRAY_( 5, 6, 7 ) );
    Date date( "2020-01-01T00:00:00+0000" );

    checkDaysNotInRange( event, date, 4 );
    checkDaysInRange( event, date, 3 );
    checkDaysNotInRange( event, date, Date::getDaysInMonth( 1 ) - 7 );

    EXPECT_EQ( date, Date( "2020-02-01T00:00:00+0000" ) );
}

TEST_F( PeriodicDateRangeEventsTests, twoSeparateDaysOfMonth ) {  // 4.3
    const int day1 = 6;
    const int day2 = 23;
    DaysOfPeriodEvent event( DaysOfPeriodEvent::Period::Month, JSON_ARRAY_( day1, day2 ) );
    Date date( "2020-01-01T00:00:00+0000" );

    checkDaysNotInRange( event, date, day1 - 1 );
    checkDaysInRange( event, date, 1 );
    checkDaysNotInRange( event, date, day2 - day1 - 1 );
    checkDaysInRange( event, date, 1 );
    checkDaysNotInRange( event, date, Date::getDaysInMonth( 1 ) - day2 );

    EXPECT_EQ( date, Date( "2020-02-01T00:00:00+0000" ) );
}

TEST_F( PeriodicDateRangeEventsTests, invalidDayOfMonthNumber ) {  // 4.4
    auto period = DaysOfPeriodEvent::Period::Month;
    EXPECT_THROW( DaysOfPeriodEvent( period, JSON_ARRAY_( 33 ) ), SceneException );
}

TEST_F( PeriodicDateRangeEventsTests, weekOfMonthNextExecutionDate ) {  // 5.1
    using namespace GregorianCalendar;
    WeeksOfPeriodEvent event( WeeksOfPeriodEvent::Period::Month, JSON_ARRAY_( 2 ) );
    Date date( "2020-01-01T00:00:00+0000" );

    auto nextExecutionDate = event.getNextDate( date.toTimePoint(), date.getTimeZoneOffset() );
    EXPECT_EQ( Date( TimePoint( nextExecutionDate->duration() ) ).toString(), "2020-02-01T00:00:00+0000" );

    date.incrementMonth();
    nextExecutionDate = event.getNextDate( date.toTimePoint(), date.getTimeZoneOffset() );
    EXPECT_EQ( Date( TimePoint( nextExecutionDate->duration() ) ).toString(), "2020-03-01T00:00:00+0000" );
}

TEST_F( PeriodicDateRangeEventsTests, weekOfYearNextExecutionDate ) {  // 5.2
    using namespace GregorianCalendar;
    WeeksOfPeriodEvent event( WeeksOfPeriodEvent::Period::Year, JSON_ARRAY_( 2 ) );
    Date date( "2020-01-01T00:00:00+0000" );

    auto nextExecutionDate = event.getNextDate( date.toTimePoint(), date.getTimeZoneOffset() );
    EXPECT_EQ( Date( TimePoint( nextExecutionDate->duration() ) ).toString(), "2021-01-01T00:00:00+0000" );

    date.incrementYear();
    nextExecutionDate = event.getNextDate( date.toTimePoint(), date.getTimeZoneOffset() );
    EXPECT_EQ( Date( TimePoint( nextExecutionDate->duration() ) ).toString(), "2022-01-01T00:00:00+0000" );
}

TEST_F( PeriodicDateRangeEventsTests, dayOfMonthNextExecutionDate ) {  // 5.3
    using namespace GregorianCalendar;
    DaysOfPeriodEvent event( DaysOfPeriodEvent::Period::Month, JSON_ARRAY_( 2 ) );
    Date date( "2020-01-01T00:00:00+0000" );

    auto nextExecutionDate = event.getNextDate( date.toTimePoint(), date.getTimeZoneOffset() );
    EXPECT_EQ( Date( TimePoint( nextExecutionDate->duration() ) ).toString(), "2020-01-02T00:00:00+0000" );

    date.incrementMonth();
    nextExecutionDate = event.getNextDate( date.toTimePoint(), date.getTimeZoneOffset() );
    EXPECT_EQ( Date( TimePoint( nextExecutionDate->duration() ) ).toString(), "2020-02-02T00:00:00+0000" );
}

TEST_F( PeriodicDateRangeEventsTests, dayOfWeekNextExecutionDate ) {  // 5.4
    using namespace GregorianCalendar;
    DaysOfPeriodEvent event( DaysOfPeriodEvent::Period::Week, JSON_ARRAY_( 2 ) );
    Date date( "2020-01-01T00:00:00+0000" );

    auto nextExecutionDate = event.getNextDate( date.toTimePoint(), date.getTimeZoneOffset() );
    EXPECT_EQ( Date( TimePoint( nextExecutionDate->duration() ) ).toString(), "2020-01-07T00:00:00+0000" );

    date.incrementWeek();
    nextExecutionDate = event.getNextDate( date.toTimePoint(), date.getTimeZoneOffset() );
    EXPECT_EQ( Date( TimePoint( nextExecutionDate->duration() ) ).toString(), "2020-01-14T00:00:00+0000" );
}

}  // namespace UT
}  // namespace SceneEngine
