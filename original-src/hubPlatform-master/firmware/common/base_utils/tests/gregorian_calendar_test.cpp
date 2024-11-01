#include "base_utils/gregorian_calendar.h"
#include "base_utils/timeutils.h"
#include "base_utils/gregorian_calendar_weeks.h"
#include "common_utils/cpp14.h"
#include "log_utils/ezlo_log.h"
#include <gtest/gtest.h>

std::string toStandardString( const GregorianCalendar::Date& date ) {
    return date.toString( "%c %z, wday: %u" );
}

namespace GregorianCalendar {
void PrintTo( const Date& date, std::ostream* os ) {
    *os << toStandardString( date );
}
}  // namespace GregorianCalendar

std::ostream& operator<<( std::ostream& os, const GregorianCalendar::Date& date ) {
    os << date.toString( "%Ft%T%z, %A (wday: %u)" );
    return os;
}

namespace HubPlatform {
namespace UT {

using namespace GregorianCalendar;

class YearTestSuite : public ::testing::Test {};
class MonthTestSuite : public ::testing::Test {
public:
    void SetUp() override {
        m_year = std::make_unique< Year >( 2019 );
    }
    Year& getYear() {
        return *m_year;
    }

private:
    std::unique_ptr< Year > m_year;
};

class DayTestSuite : public MonthTestSuite {
public:
    void SetUp() override {
        MonthTestSuite::SetUp();
        m_month = std::make_unique< Month >( getYear() );
    }
    Month& getMonth() {
        return *m_month;
    }

private:
    std::unique_ptr< Month > m_month;
};

class WeekdayTestSuite : public DayTestSuite {
public:
    void SetUp() override {
        DayTestSuite::SetUp();
        m_day = std::make_unique< Day >( getMonth() );
    }
    Day& getDay() {
        return *m_day;
    }

private:
    std::unique_ptr< Day > m_day;
};

class HourTestSuite : public DayTestSuite {
public:
    void SetUp() override {
        DayTestSuite::SetUp();
        m_day = std::make_unique< Day >( getMonth() );
    }
    Day& getDay() {
        return *m_day;
    }

private:
    std::unique_ptr< Day > m_day;
};

class MinuteTestSuite : public HourTestSuite {
public:
    void SetUp() override {
        HourTestSuite::SetUp();
        m_hour = std::make_unique< Hour >( getDay() );
    }
    Hour& getHour() {
        return *m_hour;
    }

private:
    std::unique_ptr< Hour > m_hour;
};

class SecondTestSuite : public MinuteTestSuite {
public:
    void SetUp() override {
        MinuteTestSuite::SetUp();
        m_minute = std::make_unique< Minute >( getHour() );
    }
    Minute& getMinute() {
        return *m_minute;
    }

private:
    std::unique_ptr< Minute > m_minute;
};

class MillisecondTestSuite : public SecondTestSuite {
public:
    void SetUp() override {
        SecondTestSuite::SetUp();
        m_second = std::make_unique< Second >( getMinute() );
    }
    Second& getSecond() {
        return *m_second;
    }

private:
    std::unique_ptr< Second > m_second;
};

class DateTestSuite : public ::testing::Test {};

/*-------------------------------------------------------------------------------------------------------------------*/
TEST_F( YearTestSuite, getYear ) {
    EXPECT_EQ( Year( 3 ).getUnit(), 3 );
    EXPECT_EQ( Year( 4 ).getUnit(), 4 );
    EXPECT_EQ( Year( 8 ).getUnit(), 8 );
    EXPECT_EQ( Year( 100 ).getUnit(), 100 );
    EXPECT_EQ( Year( 400 ).getUnit(), 400 );
}

TEST_F( YearTestSuite, getLeapYearsNumber ) {
    int leapYears = 0;
    for( int year = 0; year < 4000; ++year ) {
        EXPECT_EQ( Year::getLeapYearsNumber( year ), leapYears );
        if( Year::isLeapYear( year ) ) {
            ++leapYears;
        }
    }
    leapYears = 0;
    for( int year = 0; year > -4000; --year ) {
        EXPECT_EQ( Year::getLeapYearsNumber( year ), leapYears );
        if( Year::isLeapYear( year ) ) {
            --leapYears;
        }
    }
}

TEST_F( YearTestSuite, isLeapYear ) {
    EXPECT_FALSE( Year( 3 ).isLeapYear() );
    EXPECT_TRUE( Year( 4 ).isLeapYear() );
    EXPECT_TRUE( Year( 8 ).isLeapYear() );
    EXPECT_FALSE( Year( 100 ).isLeapYear() );
    EXPECT_TRUE( Year( 400 ).isLeapYear() );
    EXPECT_TRUE( Year( 2020 ).isLeapYear() );
}

TEST_F( YearTestSuite, increment ) {
    const int startYear = -3000;
    Year year( startYear );
    for( int i = startYear; i < 0; ++i, ++year ) {
        EXPECT_EQ( year.getUnit(), i );
        EXPECT_EQ( year.getLeapYearsNumber(), Year::getLeapYearsNumber( i ) );
    }
    for( int i = 1; i < -startYear; ++i, ++year ) {
        EXPECT_EQ( year.getUnit(), i );
        EXPECT_EQ( year.getLeapYearsNumber(), Year::getLeapYearsNumber( i ) );
    }
}

TEST_F( YearTestSuite, decrement ) {
    const int startYear = 3000;
    Year year( startYear );
    for( int i = startYear; i > 0; --i, --year ) {
        EXPECT_EQ( year.getUnit(), i );
        EXPECT_EQ( year.getLeapYearsNumber(), Year::getLeapYearsNumber( i ) );
    }
    for( int i = -1; i > -startYear; --i, --year ) {
        EXPECT_EQ( year.getUnit(), i );
        EXPECT_EQ( year.getLeapYearsNumber(), Year::getLeapYearsNumber( i ) );
    }
}

TEST_F( YearTestSuite, addYears ) {
    Year year( -2 );
    EXPECT_EQ( ( year += Year( 10 ) ).getUnit(), 8 );
    EXPECT_EQ( ( year += Year( 10 ) ).getUnit(), 18 );
    EXPECT_EQ( ( year += Year( 2001 ) ).getUnit(), 2019 );
}

TEST_F( YearTestSuite, subtactYears ) {
    Year year( 2 );
    EXPECT_EQ( ( year -= Year( 10 ) ).getUnit(), -8 );
    EXPECT_EQ( ( year -= Year( 10 ) ).getUnit(), -18 );
    EXPECT_EQ( ( year -= Year( 2001 ) ).getUnit(), -2019 );
}

TEST_F( YearTestSuite, toClockDuration ) {
    using namespace std::chrono;
    EXPECT_EQ( Year( EpochYear ).toClockDuration().count(), 0 );
    EXPECT_EQ( Year( EpochYear + 1 ).toClockDuration(), ClockDuration( years( 1 ) ) );
    EXPECT_EQ( Year( EpochYear + 2 ).toClockDuration(), ClockDuration( years( 2 ) ) );
    EXPECT_EQ( Year( EpochYear + 3 ).toClockDuration(), ClockDuration( years( 2 ) + leap_years( 1 ) ) );
    EXPECT_EQ( Year( EpochYear - 1 ).toClockDuration(), ClockDuration( years( -1 ) ) );
    EXPECT_EQ( Year( EpochYear - 2 ).toClockDuration(), ClockDuration( years( -1 ) + leap_years( -1 ) ) );
    EXPECT_EQ( Year( EpochYear - 3 ).toClockDuration(), ClockDuration( years( -2 ) + leap_years( -1 ) ) );
    EXPECT_EQ( Year( 1677 ).toClockDuration(), ClockDuration( seconds( -9246096000L ) ) );
    EXPECT_EQ( Year( 2262 ).toClockDuration(), ClockDuration( seconds( 9214646400L ) ) );
    EXPECT_EQ( Year( 2019 ).toClockDuration(), ClockDuration( seconds( 1546300800L ) ) );
    EXPECT_EQ( Year( 2020 ).toClockDuration(), ClockDuration( seconds( 1577836800L ) ) );
    EXPECT_EQ( Year( 2021 ).toClockDuration(), ClockDuration( seconds( 1609459200L ) ) );
    EXPECT_EQ( Year( 2022 ).toClockDuration(), ClockDuration( seconds( 1640995200L ) ) );
    EXPECT_EQ( Year( 2023 ).toClockDuration(), ClockDuration( seconds( 1672531200L ) ) );
    EXPECT_EQ( Year( 2024 ).toClockDuration(), ClockDuration( seconds( 1704067200L ) ) );
    EXPECT_EQ( Year( 2026 ).toClockDuration(), ClockDuration( seconds( 1767225600L ) ) );
    EXPECT_EQ( Year( 2028 ).toClockDuration(), ClockDuration( seconds( 1830297600L ) ) );
    EXPECT_EQ( Year( 2031 ).toClockDuration(), ClockDuration( seconds( 1924992000L ) ) );
    EXPECT_EQ( Year( 2032 ).toClockDuration(), ClockDuration( seconds( 1956528000L ) ) );
}

TEST_F( YearTestSuite, fromSystemClock ) {
    using namespace std::chrono;
    EXPECT_EQ( Year( TimePoint( days( 18783 ) ) ).getUnit(), 2021 );
    EXPECT_EQ( Year( TimePoint( days( 18418 ) ) ).getUnit(), 2020 );
    EXPECT_EQ( Year( TimePoint( days( 18053 ) ) ).getUnit(), 2019 );
    EXPECT_EQ( Year( TimePoint( days( 17688 ) ) ).getUnit(), 2018 );
    EXPECT_EQ( Year( TimePoint( days( 17323 ) ) ).getUnit(), 2017 );
    EXPECT_EQ( Year( TimePoint( days( 0 ) ) ).getUnit(), 1970 );
    EXPECT_EQ( Year( TimePoint( days( -1 ) ) ).getUnit(), 1969 );
    EXPECT_EQ( Year( TimePoint( days( -4 ) ) ).getUnit(), 1969 );
    EXPECT_EQ( Year( TimePoint( days( -365 ) ) ).getUnit(), 1969 );
    EXPECT_EQ( Year( TimePoint( days( -366 ) ) ).getUnit(), 1968 );
    EXPECT_EQ( Year( TimePoint( days( -70053 ) ) ).getUnit(), 1778 );
    EXPECT_EQ( Year( TimePoint( days( -75053 ) ) ).getUnit(), 1764 );
}
/*-------------------------------------------------------------------------------------------------------------------*/
TEST_F( MonthTestSuite, setMonthTimePoint ) {
    using namespace std::chrono;
    Month month( getYear() );
    getYear().setUnit( TimePoint( ClockDuration( 1559834860000000000ll ) ) );
    month.setUnit( TimePoint( ClockDuration( 1559834860000000000ll ) ) );
    EXPECT_EQ( month.getUnit(), 6 );
    getYear().setUnit( TimePoint( days( 0 ) ) );
    month.setUnit( TimePoint( days( 0 ) ) );
    EXPECT_EQ( month.getUnit(), 1 );
    month.setUnit( TimePoint( days( 30 ) ) );
    EXPECT_EQ( month.getUnit(), 1 );
    month.setUnit( TimePoint( days( 31 ) ) );
    EXPECT_EQ( month.getUnit(), 2 );
    month.setUnit( TimePoint( days( 59 ) ) );
    EXPECT_EQ( month.getUnit(), 3 );
    month.setUnit( TimePoint( days( 90 ) ) );
    EXPECT_EQ( month.getUnit(), 4 );
    month.setUnit( TimePoint( days( 120 ) ) );
    EXPECT_EQ( month.getUnit(), 5 );
    month.setUnit( TimePoint( days( 151 ) ) );
    EXPECT_EQ( month.getUnit(), 6 );
    month.setUnit( TimePoint( days( 181 ) ) );
    EXPECT_EQ( month.getUnit(), 7 );
    month.setUnit( TimePoint( days( 212 ) ) );
    EXPECT_EQ( month.getUnit(), 8 );
    month.setUnit( TimePoint( days( 243 ) ) );
    EXPECT_EQ( month.getUnit(), 9 );
    month.setUnit( TimePoint( days( 273 ) ) );
    EXPECT_EQ( month.getUnit(), 10 );
    month.setUnit( TimePoint( days( 304 ) ) );
    EXPECT_EQ( month.getUnit(), 11 );
    month.setUnit( TimePoint( days( 334 ) ) );
    EXPECT_EQ( month.getUnit(), 12 );
    month.setUnit( TimePoint( days( -31 ) ) );
    EXPECT_EQ( month.getUnit(), 12 );
    month.setUnit( TimePoint( days( -61 ) ) );
    EXPECT_EQ( month.getUnit(), 11 );
    month.setUnit( TimePoint( days( -90 ) ) );
    EXPECT_EQ( month.getUnit(), 10 );
    month.setUnit( TimePoint( days( -120 ) ) );
    EXPECT_EQ( month.getUnit(), 9 );
    month.setUnit( TimePoint( days( -151 ) ) );
    EXPECT_EQ( month.getUnit(), 8 );
    month.setUnit( TimePoint( days( -181 ) ) );
    EXPECT_EQ( month.getUnit(), 7 );
    month.setUnit( TimePoint( days( -212 ) ) );
    EXPECT_EQ( month.getUnit(), 6 );
    month.setUnit( TimePoint( days( -243 ) ) );
    EXPECT_EQ( month.getUnit(), 5 );
    month.setUnit( TimePoint( days( -273 ) ) );
    EXPECT_EQ( month.getUnit(), 4 );
    month.setUnit( TimePoint( days( -304 ) ) );
    EXPECT_EQ( month.getUnit(), 3 );
    month.setUnit( TimePoint( days( -334 ) ) );
    EXPECT_EQ( month.getUnit(), 2 );
    month.setUnit( TimePoint( days( -365 ) ) );
    EXPECT_EQ( month.getUnit(), 1 );
    getYear().setUnit( TimePoint( ClockDuration( 68225260000000000ll ) ) );
    month.setUnit( TimePoint( ClockDuration( 68225260000000000ll ) ) );
    EXPECT_EQ( month.getUnit(), 2 );
}

TEST_F( MonthTestSuite, setMonthNumber ) {
    Month month( getYear() );
    EXPECT_TRUE( month.setUnit( 12 ) );
    EXPECT_EQ( month.getUnit(), 12 );
    EXPECT_TRUE( month.setUnit( 11 ) );
    EXPECT_EQ( month.getUnit(), 11 );
    EXPECT_TRUE( month.setUnit( 6 ) );
    EXPECT_EQ( month.getUnit(), 6 );
    EXPECT_TRUE( month.setUnit( 1 ) );
    EXPECT_EQ( month.getUnit(), 1 );
    EXPECT_FALSE( month.setUnit( 0 ) );
    EXPECT_EQ( month.getUnit(), 1 );
    EXPECT_FALSE( month.setUnit( -10 ) );
    EXPECT_EQ( month.getUnit(), 1 );
    EXPECT_FALSE( month.setUnit( 13 ) );
    EXPECT_EQ( month.getUnit(), 1 );
    EXPECT_FALSE( month.setUnit( 100 ) );
    EXPECT_EQ( month.getUnit(), 1 );
}

TEST_F( MonthTestSuite, increment ) {
    Month month( getYear() );
    for( int i = 1; i != 12; ++i, ++month ) {
        EXPECT_EQ( month.getUnit(), i );
        EXPECT_EQ( getYear().getUnit(), 2019 );
    }
    EXPECT_EQ( ( ++month ).getUnit(), 1 );
    EXPECT_EQ( getYear().getUnit(), 2020 );
}

TEST_F( MonthTestSuite, decrement ) {
    Month month( getYear() );
    EXPECT_EQ( month.getUnit(), 1 );
    --month;
    for( int i = 12; i != 1; --i, --month ) {
        EXPECT_EQ( month.getUnit(), i );
        EXPECT_EQ( getYear().getUnit(), 2018 );
    }
    EXPECT_EQ( ( --month ).getUnit(), 12 );
    EXPECT_EQ( getYear().getUnit(), 2017 );
}

TEST_F( MonthTestSuite, addMonths ) {
    Month month( getYear() );
    month += 6;
    EXPECT_EQ( month.getUnit(), 7 );
    EXPECT_EQ( getYear().getUnit(), 2019 );
    month += 6;
    EXPECT_EQ( month.getUnit(), 1 );
    EXPECT_EQ( getYear().getUnit(), 2020 );
    month += 13;
    EXPECT_EQ( month.getUnit(), 2 );
    EXPECT_EQ( getYear().getUnit(), 2021 );
    month += 66;
    EXPECT_EQ( month.getUnit(), 8 );
    EXPECT_EQ( getYear().getUnit(), 2026 );
    month += 4;
    EXPECT_EQ( month.getUnit(), 12 );
    EXPECT_EQ( getYear().getUnit(), 2026 );
    month += 1;
    EXPECT_EQ( month.getUnit(), 1 );
    EXPECT_EQ( getYear().getUnit(), 2027 );
    month.setCurrentDayCallback( []() { return 31; } );
    month += 6;
    EXPECT_EQ( month.getUnit(), 12 );
    EXPECT_EQ( getYear().getUnit(), 2027 );
}

TEST_F( MonthTestSuite, substractMonths ) {
    Month month( getYear() );
    month -= 1;
    EXPECT_EQ( month.getUnit(), 12 );
    EXPECT_EQ( getYear().getUnit(), 2018 );
    month -= 5;
    EXPECT_EQ( month.getUnit(), 7 );
    EXPECT_EQ( getYear().getUnit(), 2018 );
    month -= 7;
    EXPECT_EQ( month.getUnit(), 12 );
    EXPECT_EQ( getYear().getUnit(), 2017 );
    month -= 67;
    EXPECT_EQ( month.getUnit(), 7 );
    EXPECT_EQ( getYear().getUnit(), 2013 );
    month.setCurrentDayCallback( []() { return 31; } );
    month -= 4;
    EXPECT_EQ( month.getUnit(), 12 );
    EXPECT_EQ( getYear().getUnit(), 2012 );
}

TEST_F( MonthTestSuite, toClockDuration ) {
    using namespace std::chrono;
    Month month( getYear() );
    EXPECT_EQ( month.toClockDuration(), days( 0 ) );
    ++month;
    EXPECT_EQ( month.toClockDuration(), days( 31 ) );
    ++month;
    EXPECT_EQ( month.toClockDuration(), days( 59 ) );
    ++month;
    EXPECT_EQ( month.toClockDuration(), days( 90 ) );
    ++month;
    EXPECT_EQ( month.toClockDuration(), days( 120 ) );
    ++month;
    EXPECT_EQ( month.toClockDuration(), days( 151 ) );
    ++month;
    EXPECT_EQ( month.toClockDuration(), days( 181 ) );
    ++month;
    EXPECT_EQ( month.toClockDuration(), days( 212 ) );
    ++month;
    EXPECT_EQ( month.toClockDuration(), days( 243 ) );
    ++month;
    EXPECT_EQ( month.toClockDuration(), days( 273 ) );
    ++month;
    EXPECT_EQ( month.toClockDuration(), days( 304 ) );
    ++month;
    EXPECT_EQ( month.toClockDuration(), days( 334 ) );
    ++month;
    EXPECT_EQ( month.toClockDuration(), days( 0 ) );
}
/*-------------------------------------------------------------------------------------------------------------------*/
TEST_F( DayTestSuite, setDayTimePoint ) {
    Day day( getMonth() );
    day.setUnit( TimePoint( ClockDuration( 1546300800000000000ll ) ) );
    EXPECT_EQ( day.getUnit(), 1 );
    day.setUnit( TimePoint( ClockDuration( 1546390800000000000ll ) ) );
    EXPECT_EQ( day.getUnit(), 2 );
    day.setUnit( TimePoint( ClockDuration( 1546477200000000000ll ) ) );
    EXPECT_EQ( day.getUnit(), 3 );
    day.setUnit( TimePoint( ClockDuration( 1547514000000000000ll ) ) );
    EXPECT_EQ( day.getUnit(), 15 );
    day.setUnit( TimePoint( ClockDuration( 1548896400000000000ll ) ) );
    EXPECT_EQ( day.getUnit(), 31 );
    day.setUnit( TimePoint( ClockDuration( 1548982800000000000ll ) ) );
    EXPECT_EQ( day.getUnit(), 1 );
}

TEST_F( DayTestSuite, setDayNumber ) {
    Day day( getMonth() );
    EXPECT_FALSE( day.setUnit( 0 ) );
    EXPECT_FALSE( day.setUnit( -10 ) );
    EXPECT_FALSE( day.setUnit( 32 ) );
    EXPECT_FALSE( day.setUnit( 100 ) );
    EXPECT_TRUE( day.setUnit( 1 ) );
    EXPECT_EQ( day.getUnit(), 1 );
    EXPECT_TRUE( day.setUnit( 15 ) );
    EXPECT_EQ( day.getUnit(), 15 );
    EXPECT_TRUE( day.setUnit( 31 ) );
    EXPECT_EQ( day.getUnit(), 31 );
    EXPECT_TRUE( day.setUnit( 28 ) );
    EXPECT_EQ( day.getUnit(), 28 );
    ++getMonth();
    EXPECT_EQ( day.getUnit(), 28 );
}

TEST_F( DayTestSuite, increment ) {
    Day day( getMonth() );
    ++day;
    EXPECT_EQ( day.getUnit(), 2 );
    ++day;
    EXPECT_EQ( day.getUnit(), 3 );
    day.setUnit( 31 );
    ++day;
    EXPECT_EQ( day.getUnit(), 1 );
    EXPECT_EQ( getMonth().getUnit(), 2 );
}

TEST_F( DayTestSuite, decrement ) {
    Day day( getMonth() );
    --day;
    EXPECT_EQ( day.getUnit(), 31 );
    EXPECT_EQ( getMonth().getUnit(), 12 );
    EXPECT_EQ( getYear().getUnit(), 2018 );
    --day;
    EXPECT_EQ( day.getUnit(), 30 );
    --day;
    EXPECT_EQ( day.getUnit(), 29 );
    day.setUnit( 1 );
    --day;
    EXPECT_EQ( day.getUnit(), 30 );
    EXPECT_EQ( getMonth().getUnit(), 11 );
}

TEST_F( DayTestSuite, addDays ) {
    Day day( getMonth() );
    day += 5;
    EXPECT_EQ( day.getUnit(), 6 );
    day += 10;
    EXPECT_EQ( day.getUnit(), 16 );
    day += 20;
    EXPECT_EQ( day.getUnit(), 5 );
    EXPECT_EQ( getMonth().getUnit(), 2 );
    day += 23;
    EXPECT_EQ( day.getUnit(), 28 );
    day += 1;
    EXPECT_EQ( day.getUnit(), 1 );
    EXPECT_EQ( getMonth().getUnit(), 3 );
    day += 305;
    EXPECT_EQ( day.getUnit(), 31 );
    EXPECT_EQ( getMonth().getUnit(), 12 );
    day += 1;
    EXPECT_EQ( day.getUnit(), 1 );
    EXPECT_EQ( getMonth().getUnit(), 1 );
    EXPECT_EQ( getYear().getUnit(), 2020 );
}

TEST_F( DayTestSuite, substractDays ) {
    Day day( getMonth() );
    day -= 1;
    EXPECT_EQ( day.getUnit(), 31 );
    EXPECT_EQ( getMonth().getUnit(), 12 );
    EXPECT_EQ( getYear().getUnit(), 2018 );
    day -= 15;
    EXPECT_EQ( day.getUnit(), 16 );
    EXPECT_EQ( getMonth().getUnit(), 12 );
    day -= 16;
    EXPECT_EQ( day.getUnit(), 30 );
    EXPECT_EQ( getMonth().getUnit(), 11 );
    day -= 334;
    EXPECT_EQ( day.getUnit(), 31 );
    EXPECT_EQ( getMonth().getUnit(), 12 );
    EXPECT_EQ( getYear().getUnit(), 2017 );
}

TEST_F( DayTestSuite, toClockDuration ) {
    using namespace std::chrono;
    Day day( getMonth() );
    EXPECT_EQ( day.toClockDuration(), days( 0 ) );
    ++day;
    EXPECT_EQ( day.toClockDuration(), days( 1 ) );
    ++day;
    EXPECT_EQ( day.toClockDuration(), days( 2 ) );
    day += 28;
    EXPECT_EQ( day.toClockDuration(), days( 30 ) );
    day += 1;
    EXPECT_EQ( day.toClockDuration(), days( 0 ) );
    day += 15;
    EXPECT_EQ( day.toClockDuration(), days( 15 ) );
    day += 13;
    EXPECT_EQ( day.toClockDuration(), days( 0 ) );
}

/*-------------------------------------------------------------------------------------------------------------------*/
TEST_F( WeekdayTestSuite, setTimePoint ) {
    Weekday weekday( getDay() );
    weekday.setUnit( TimePoint( ClockDuration( 0 ) ) );
    EXPECT_EQ( weekday.getUnit(), 4 );
    weekday.setUnit( TimePoint( ClockDuration( 86400000000000ll ) ) );
    EXPECT_EQ( weekday.getUnit(), 5 );
    weekday.setUnit( TimePoint( ClockDuration( 259200000000000ll ) ) );
    EXPECT_EQ( weekday.getUnit(), 7 );
    weekday.setUnit( TimePoint( ClockDuration( 345600000000000ll ) ) );
    EXPECT_EQ( weekday.getUnit(), 1 );
    weekday.setUnit( TimePoint( ClockDuration( 1546646400000000000ll ) ) );
    EXPECT_EQ( weekday.getUnit(), 6 );
    weekday.setUnit( TimePoint( ClockDuration( 4102790400000000000ll ) ) );
    EXPECT_EQ( weekday.getUnit(), 2 );
    weekday.setUnit( TimePoint( ClockDuration( 4134326400000000000ll ) ) );
    EXPECT_EQ( weekday.getUnit(), 3 );
    weekday.setUnit( TimePoint( ClockDuration( 1546300800000000000ll ) ) );
    EXPECT_EQ( weekday.getUnit(), 2 );
}

TEST_F( WeekdayTestSuite, setWeekdayNumber ) {
    Weekday weekday( getDay() );
    weekday.setUnit( TimePoint( ClockDuration( 1546300800000000000ll ) ) );
    EXPECT_FALSE( weekday.setUnit( 0 ) );
    EXPECT_EQ( weekday.getUnit(), 2 );
    EXPECT_FALSE( weekday.setUnit( 8 ) );
    EXPECT_EQ( weekday.getUnit(), 2 );
    EXPECT_FALSE( weekday.setUnit( 100 ) );
    EXPECT_EQ( weekday.getUnit(), 2 );
    EXPECT_TRUE( weekday.setUnit( 2 ) );
    EXPECT_EQ( weekday.getUnit(), 2 );
    EXPECT_TRUE( weekday.setUnit( 1 ) );
    EXPECT_EQ( weekday.getUnit(), 1 );
    for( int i = 2; i <= 7; ++i ) {
        EXPECT_TRUE( weekday.setUnit( i ) );
        EXPECT_EQ( weekday.getUnit(), i );
    }
}

TEST_F( WeekdayTestSuite, moveToWeekday ) {
    Weekday weekday( getDay() );
    weekday.setUnit( TimePoint( Date( "2019-01-01T00:00:00+0000" ).toClockDuration() ) );
    for( int i = 2; i <= 7; ++i ) {
        EXPECT_TRUE( weekday.moveToWeekday( i ) );
        EXPECT_EQ( weekday.getUnit(), i );
        EXPECT_EQ( getDay().getUnit(), i - 1 );
    }
}

TEST_F( WeekdayTestSuite, increment ) {
    Weekday weekday( getDay() );
    weekday.setUnit( TimePoint( ClockDuration( 1546300800000000000ll ) ) );
    for( int i = 2; i != 7; ++i, ++weekday ) {
        EXPECT_EQ( weekday.getUnit(), i );
        EXPECT_EQ( getDay().getUnit(), i - 1 );
    }
}

TEST_F( WeekdayTestSuite, decrement ) {
    Weekday weekday( getDay() );
    weekday.setUnit( TimePoint( ClockDuration( 1546300800000000000ll ) ) );
    --weekday;
    --weekday;
    for( int i = 7, day = 30; i != 1; --i, --weekday, --day ) {
        EXPECT_EQ( weekday.getUnit(), i );
        EXPECT_EQ( getDay().getUnit(), day );
    }
}

TEST_F( WeekdayTestSuite, add ) {
    Weekday weekday( getDay() );
    weekday.setUnit( TimePoint( ClockDuration( 1546300800000000000ll ) ) );
    weekday += 7;
    EXPECT_EQ( weekday.getUnit(), 2 );
    EXPECT_EQ( getDay().getUnit(), 8 );
    weekday += 16;
    EXPECT_EQ( weekday.getUnit(), 4 );
    EXPECT_EQ( getDay().getUnit(), 24 );
}

TEST_F( WeekdayTestSuite, substract ) {
    Weekday weekday( getDay() );
    weekday.setUnit( TimePoint( ClockDuration( 1546300800000000000ll ) ) );
    weekday -= 1;
    EXPECT_EQ( weekday.getUnit(), 1 );
    EXPECT_EQ( getDay().getUnit(), 31 );
    weekday -= 1;
    EXPECT_EQ( weekday.getUnit(), 7 );
    EXPECT_EQ( getDay().getUnit(), 30 );
    weekday -= 7;
    EXPECT_EQ( weekday.getUnit(), 7 );
    EXPECT_EQ( getDay().getUnit(), 23 );
    weekday -= 16;
    EXPECT_EQ( weekday.getUnit(), 5 );
    EXPECT_EQ( getDay().getUnit(), 7 );
}

/*-------------------------------------------------------------------------------------------------------------------*/
TEST_F( HourTestSuite, setHourTimePoint ) {
    Hour hour( getDay() );
    hour.setUnit( TimePoint( ClockDuration( 1546300800000000000ll ) ) );
    EXPECT_EQ( hour.getUnit(), 0 );
    hour.setUnit( TimePoint( ClockDuration( 1546304400000000000ll ) ) );
    EXPECT_EQ( hour.getUnit(), 1 );
    hour.setUnit( TimePoint( ClockDuration( 1546344000000000000ll ) ) );
    EXPECT_EQ( hour.getUnit(), 12 );
    hour.setUnit( TimePoint( ClockDuration( 1546383600000000000ll ) ) );
    EXPECT_EQ( hour.getUnit(), 23 );
    hour.setUnit( TimePoint( ClockDuration( 1546387200000000000ll ) ) );
    EXPECT_EQ( hour.getUnit(), 0 );
}

TEST_F( HourTestSuite, setHourNumber ) {
    Hour hour( getDay() );
    EXPECT_TRUE( hour.setUnit( 1 ) );
    EXPECT_EQ( hour.getUnit(), 1 );
    EXPECT_TRUE( hour.setUnit( 2 ) );
    EXPECT_EQ( hour.getUnit(), 2 );
    EXPECT_TRUE( hour.setUnit( 12 ) );
    EXPECT_EQ( hour.getUnit(), 12 );
    EXPECT_TRUE( hour.setUnit( 23 ) );
    EXPECT_EQ( hour.getUnit(), 23 );
    EXPECT_FALSE( hour.setUnit( 24 ) );
    EXPECT_EQ( hour.getUnit(), 23 );
    EXPECT_FALSE( hour.setUnit( 100 ) );
    EXPECT_EQ( hour.getUnit(), 23 );
    EXPECT_FALSE( hour.setUnit( -1 ) );
    EXPECT_EQ( hour.getUnit(), 23 );
    EXPECT_FALSE( hour.setUnit( -23 ) );
    EXPECT_EQ( hour.getUnit(), 23 );
}

TEST_F( HourTestSuite, increment ) {
    Hour hour( getDay() );
    for( int i = 0; i != 23; ++i, ++hour ) {
        EXPECT_EQ( hour.getUnit(), i );
        EXPECT_EQ( getDay().getUnit(), 1 );
    }
    ++hour;
    EXPECT_EQ( hour.getUnit(), 0 );
    EXPECT_EQ( getDay().getUnit(), 2 );
}

TEST_F( HourTestSuite, decrement ) {
    Hour hour( getDay() );
    --hour;
    for( int i = 23; i != 0; --i, --hour ) {
        EXPECT_EQ( hour.getUnit(), i );
        EXPECT_EQ( getDay().getUnit(), 31 );
    }
    --hour;
    EXPECT_EQ( hour.getUnit(), 23 );
    EXPECT_EQ( getDay().getUnit(), 30 );
}

TEST_F( HourTestSuite, addHours ) {
    Hour hour( getDay() );
    hour += 12;
    EXPECT_EQ( hour.getUnit(), 12 );
    hour += 11;
    EXPECT_EQ( hour.getUnit(), 23 );
    EXPECT_EQ( getDay().getUnit(), 1 );
    hour += 1;
    EXPECT_EQ( hour.getUnit(), 0 );
    EXPECT_EQ( getDay().getUnit(), 2 );
    hour += 24;
    EXPECT_EQ( hour.getUnit(), 0 );
    EXPECT_EQ( getDay().getUnit(), 3 );
    hour += 120;
    EXPECT_EQ( hour.getUnit(), 0 );
    EXPECT_EQ( getDay().getUnit(), 8 );
}

TEST_F( HourTestSuite, substractHours ) {
    Hour hour( getDay() );
    hour -= 1;
    EXPECT_EQ( hour.getUnit(), 23 );
    EXPECT_EQ( getDay().getUnit(), 31 );
    EXPECT_EQ( getMonth().getUnit(), 12 );
    hour -= 15;
    EXPECT_EQ( hour.getUnit(), 8 );
    EXPECT_EQ( getDay().getUnit(), 31 );
    hour -= 8;
    EXPECT_EQ( hour.getUnit(), 0 );
    EXPECT_EQ( getDay().getUnit(), 31 );
    hour -= 24;
    EXPECT_EQ( hour.getUnit(), 0 );
    EXPECT_EQ( getDay().getUnit(), 30 );
    hour -= 120;
    EXPECT_EQ( hour.getUnit(), 0 );
    EXPECT_EQ( getDay().getUnit(), 25 );
}

TEST_F( HourTestSuite, toClockDuration ) {
    using namespace std::chrono;
    Hour hour( getDay() );
    EXPECT_EQ( hour.toClockDuration(), hours( 0 ) );
    ++hour;
    EXPECT_EQ( hour.toClockDuration(), hours( 1 ) );
    ++hour;
    EXPECT_EQ( hour.toClockDuration(), hours( 2 ) );
    hour += 21;
    EXPECT_EQ( hour.toClockDuration(), hours( 23 ) );
    hour += 1;
    EXPECT_EQ( hour.toClockDuration(), hours( 0 ) );
    hour += 15;
    EXPECT_EQ( hour.toClockDuration(), hours( 15 ) );
    hour += 9;
    EXPECT_EQ( hour.toClockDuration(), hours( 0 ) );
}
/*-------------------------------------------------------------------------------------------------------------------*/
TEST_F( MinuteTestSuite, setMinuteTimePoint ) {
    Minute minute( getHour() );
    minute.setUnit( TimePoint( ClockDuration( 1546300800000000000ll ) ) );
    EXPECT_EQ( minute.getUnit(), 0 );
    minute.setUnit( TimePoint( ClockDuration( 1546300860000000000ll ) ) );
    EXPECT_EQ( minute.getUnit(), 1 );
    minute.setUnit( TimePoint( ClockDuration( 1546302300000000000ll ) ) );
    EXPECT_EQ( minute.getUnit(), 25 );
    minute.setUnit( TimePoint( ClockDuration( 1546303500000000000ll ) ) );
    EXPECT_EQ( minute.getUnit(), 45 );
    minute.setUnit( TimePoint( ClockDuration( 1546304340000000000ll ) ) );
    EXPECT_EQ( minute.getUnit(), 59 );
    minute.setUnit( TimePoint( ClockDuration( 1546304400000000000ll ) ) );
    EXPECT_EQ( minute.getUnit(), 0 );
    minute.setUnit( TimePoint( ClockDuration( 1546387200000000000ll ) ) );
    EXPECT_EQ( minute.getUnit(), 0 );
}

TEST_F( MinuteTestSuite, setMinuteNumber ) {
    Minute minute( getHour() );
    EXPECT_TRUE( minute.setUnit( 1 ) );
    EXPECT_EQ( minute.getUnit(), 1 );
    EXPECT_TRUE( minute.setUnit( 30 ) );
    EXPECT_EQ( minute.getUnit(), 30 );
    EXPECT_TRUE( minute.setUnit( 59 ) );
    EXPECT_EQ( minute.getUnit(), 59 );
    EXPECT_FALSE( minute.setUnit( 60 ) );
    EXPECT_EQ( minute.getUnit(), 59 );
    EXPECT_FALSE( minute.setUnit( 120 ) );
    EXPECT_EQ( minute.getUnit(), 59 );
    EXPECT_FALSE( minute.setUnit( -1 ) );
    EXPECT_EQ( minute.getUnit(), 59 );
    EXPECT_FALSE( minute.setUnit( -100 ) );
    EXPECT_EQ( minute.getUnit(), 59 );
}

TEST_F( MinuteTestSuite, addMinutes ) {
    Minute minute( getHour() );
    minute += 30;
    EXPECT_EQ( minute.getUnit(), 30 );
    minute += 29;
    EXPECT_EQ( minute.getUnit(), 59 );
    minute += 1;
    EXPECT_EQ( minute.getUnit(), 0 );
    EXPECT_EQ( getHour().getUnit(), 1 );
    minute += 60;
    EXPECT_EQ( minute.getUnit(), 0 );
    EXPECT_EQ( getHour().getUnit(), 2 );
    minute += 300;
    EXPECT_EQ( minute.getUnit(), 0 );
    EXPECT_EQ( getHour().getUnit(), 7 );
}

TEST_F( MinuteTestSuite, substractMinutes ) {
    Minute minute( getHour() );
    minute -= 1;
    EXPECT_EQ( minute.getUnit(), 59 );
    EXPECT_EQ( getHour().getUnit(), 23 );
    minute -= 30;
    EXPECT_EQ( minute.getUnit(), 29 );
    EXPECT_EQ( getHour().getUnit(), 23 );
    minute -= 29;
    EXPECT_EQ( minute.getUnit(), 0 );
    EXPECT_EQ( getHour().getUnit(), 23 );
    minute -= 60;
    EXPECT_EQ( minute.getUnit(), 0 );
    EXPECT_EQ( getHour().getUnit(), 22 );
    minute -= 300;
    EXPECT_EQ( minute.getUnit(), 0 );
    EXPECT_EQ( getHour().getUnit(), 17 );
}

TEST_F( MinuteTestSuite, increment ) {
    Minute minute( getHour() );
    ++minute;
    EXPECT_EQ( minute.getUnit(), 1 );
    ++minute;
    EXPECT_EQ( minute.getUnit(), 2 );
    ++minute;
    EXPECT_EQ( minute.getUnit(), 3 );
    minute += 56;
    EXPECT_EQ( minute.getUnit(), 59 );
    ++minute;
    EXPECT_EQ( minute.getUnit(), 0 );
    EXPECT_EQ( getHour().getUnit(), 1 );
}

TEST_F( MinuteTestSuite, decrement ) {
    Minute minute( getHour() );
    --minute;
    EXPECT_EQ( minute.getUnit(), 59 );
    EXPECT_EQ( getHour().getUnit(), 23 );
    --minute;
    EXPECT_EQ( minute.getUnit(), 58 );
    EXPECT_EQ( getHour().getUnit(), 23 );
    --minute;
    EXPECT_EQ( minute.getUnit(), 57 );
    EXPECT_EQ( getHour().getUnit(), 23 );
    minute -= 57;
    EXPECT_EQ( minute.getUnit(), 0 );
    --minute;
    EXPECT_EQ( minute.getUnit(), 59 );
    EXPECT_EQ( getHour().getUnit(), 22 );
}

TEST_F( MinuteTestSuite, toClockDuration ) {
    using namespace std::chrono;
    Minute minute( getHour() );
    EXPECT_EQ( minute.toClockDuration(), minutes( 0 ) );
    ++minute;
    EXPECT_EQ( minute.toClockDuration(), minutes( 1 ) );
    ++minute;
    EXPECT_EQ( minute.toClockDuration(), minutes( 2 ) );
    minute += 57;
    EXPECT_EQ( minute.toClockDuration(), minutes( 59 ) );
    minute += 1;
    EXPECT_EQ( minute.toClockDuration(), minutes( 0 ) );
    minute += 30;
    EXPECT_EQ( minute.toClockDuration(), minutes( 30 ) );
    minute += 30;
    EXPECT_EQ( minute.toClockDuration(), minutes( 0 ) );
}

/*-------------------------------------------------------------------------------------------------------------------*/
TEST_F( SecondTestSuite, setTimePoint ) {
    Second second( getMinute() );
    second.setUnit( TimePoint( ClockDuration( 1546300800000000000ll ) ) );
    EXPECT_EQ( second.getUnit(), 0 );
    second.setUnit( TimePoint( ClockDuration( 1546300801000000000ll ) ) );
    EXPECT_EQ( second.getUnit(), 1 );
    second.setUnit( TimePoint( ClockDuration( 1546300830000000000ll ) ) );
    EXPECT_EQ( second.getUnit(), 30 );
    second.setUnit( TimePoint( ClockDuration( 1546300859000000000ll ) ) );
    EXPECT_EQ( second.getUnit(), 59 );
    second.setUnit( TimePoint( ClockDuration( 1546300860000000000ll ) ) );
    EXPECT_EQ( second.getUnit(), 0 );
    second.setUnit( TimePoint( ClockDuration( 1546304340000000000ll ) ) );
    EXPECT_EQ( second.getUnit(), 0 );
}

TEST_F( SecondTestSuite, setUnit ) {
    Second second( getMinute() );
    EXPECT_TRUE( second.setUnit( 1 ) );
    EXPECT_EQ( second.getUnit(), 1 );
    EXPECT_TRUE( second.setUnit( 30 ) );
    EXPECT_EQ( second.getUnit(), 30 );
    EXPECT_TRUE( second.setUnit( 59 ) );
    EXPECT_EQ( second.getUnit(), 59 );
    EXPECT_FALSE( second.setUnit( 60 ) );
    EXPECT_EQ( second.getUnit(), 59 );
    EXPECT_FALSE( second.setUnit( 120 ) );
    EXPECT_EQ( second.getUnit(), 59 );
    EXPECT_FALSE( second.setUnit( -1 ) );
    EXPECT_EQ( second.getUnit(), 59 );
    EXPECT_FALSE( second.setUnit( -100 ) );
    EXPECT_EQ( second.getUnit(), 59 );
}

TEST_F( SecondTestSuite, addUnit ) {
    Second second( getMinute() );
    second += 30;
    EXPECT_EQ( second.getUnit(), 30 );
    second += 29;
    EXPECT_EQ( second.getUnit(), 59 );
    second += 1;
    EXPECT_EQ( second.getUnit(), 0 );
    EXPECT_EQ( getMinute().getUnit(), 1 );
    second += 60;
    EXPECT_EQ( second.getUnit(), 0 );
    EXPECT_EQ( getMinute().getUnit(), 2 );
    second += 300;
    EXPECT_EQ( second.getUnit(), 0 );
    EXPECT_EQ( getMinute().getUnit(), 7 );
}

TEST_F( SecondTestSuite, substractUnit ) {
    Second second( getMinute() );
    second -= 1;
    EXPECT_EQ( second.getUnit(), 59 );
    EXPECT_EQ( getMinute().getUnit(), 59 );
    second -= 30;
    EXPECT_EQ( second.getUnit(), 29 );
    EXPECT_EQ( getMinute().getUnit(), 59 );
    second -= 29;
    EXPECT_EQ( second.getUnit(), 0 );
    EXPECT_EQ( getMinute().getUnit(), 59 );
    second -= 60;
    EXPECT_EQ( second.getUnit(), 0 );
    EXPECT_EQ( getMinute().getUnit(), 58 );
    second -= 300;
    EXPECT_EQ( second.getUnit(), 0 );
    EXPECT_EQ( getMinute().getUnit(), 53 );
}

TEST_F( SecondTestSuite, increment ) {
    Second second( getMinute() );
    ++second;
    EXPECT_EQ( second.getUnit(), 1 );
    ++second;
    EXPECT_EQ( second.getUnit(), 2 );
    ++second;
    EXPECT_EQ( second.getUnit(), 3 );
    second += 56;
    EXPECT_EQ( second.getUnit(), 59 );
    ++second;
    EXPECT_EQ( second.getUnit(), 0 );
    EXPECT_EQ( getMinute().getUnit(), 1 );
}

TEST_F( SecondTestSuite, decrement ) {
    Second second( getMinute() );
    --second;
    EXPECT_EQ( second.getUnit(), 59 );
    EXPECT_EQ( getMinute().getUnit(), 59 );
    --second;
    EXPECT_EQ( second.getUnit(), 58 );
    EXPECT_EQ( getMinute().getUnit(), 59 );
    --second;
    EXPECT_EQ( second.getUnit(), 57 );
    EXPECT_EQ( getMinute().getUnit(), 59 );
    second -= 57;
    EXPECT_EQ( second.getUnit(), 0 );
    --second;
    EXPECT_EQ( second.getUnit(), 59 );
    EXPECT_EQ( getMinute().getUnit(), 58 );
}

TEST_F( SecondTestSuite, toClockDuration ) {
    using namespace std::chrono;
    Second second( getMinute() );
    EXPECT_EQ( second.toClockDuration(), seconds( 0 ) );
    ++second;
    EXPECT_EQ( second.toClockDuration(), seconds( 1 ) );
    ++second;
    EXPECT_EQ( second.toClockDuration(), seconds( 2 ) );
    second += 57;
    EXPECT_EQ( second.toClockDuration(), seconds( 59 ) );
    second += 1;
    EXPECT_EQ( second.toClockDuration(), seconds( 0 ) );
    second += 30;
    EXPECT_EQ( second.toClockDuration(), seconds( 30 ) );
    second += 30;
    EXPECT_EQ( second.toClockDuration(), seconds( 0 ) );
}

/*-------------------------------------------------------------------------------------------------------------------*/
TEST_F( MillisecondTestSuite, setTimePoint ) {
    Millisecond millisecond( getSecond() );
    millisecond.setUnit( TimePoint( ClockDuration( 1546300800000000000ll ) ) );
    EXPECT_EQ( millisecond.getUnit(), 0 );
    millisecond.setUnit( TimePoint( ClockDuration( 1546300800001000000ll ) ) );
    EXPECT_EQ( millisecond.getUnit(), 1 );
    millisecond.setUnit( TimePoint( ClockDuration( 1546300800500000000ll ) ) );
    EXPECT_EQ( millisecond.getUnit(), 500 );
    millisecond.setUnit( TimePoint( ClockDuration( 1546300800999000000ll ) ) );
    EXPECT_EQ( millisecond.getUnit(), 999 );
    millisecond.setUnit( TimePoint( ClockDuration( 1546300801000000000ll ) ) );
    EXPECT_EQ( millisecond.getUnit(), 0 );
    millisecond.setUnit( TimePoint( ClockDuration( 1546300802000000000ll ) ) );
    EXPECT_EQ( millisecond.getUnit(), 0 );
}

TEST_F( MillisecondTestSuite, setUnit ) {
    Millisecond millisecond( getSecond() );
    EXPECT_TRUE( millisecond.setUnit( 1 ) );
    EXPECT_EQ( millisecond.getUnit(), 1 );
    EXPECT_TRUE( millisecond.setUnit( 500 ) );
    EXPECT_EQ( millisecond.getUnit(), 500 );
    EXPECT_TRUE( millisecond.setUnit( 999 ) );
    EXPECT_EQ( millisecond.getUnit(), 999 );
    EXPECT_FALSE( millisecond.setUnit( 1000 ) );
    EXPECT_EQ( millisecond.getUnit(), 999 );
    EXPECT_FALSE( millisecond.setUnit( 5000 ) );
    EXPECT_EQ( millisecond.getUnit(), 999 );
    EXPECT_FALSE( millisecond.setUnit( -1 ) );
    EXPECT_EQ( millisecond.getUnit(), 999 );
    EXPECT_FALSE( millisecond.setUnit( -100 ) );
    EXPECT_EQ( millisecond.getUnit(), 999 );
}

TEST_F( MillisecondTestSuite, addUnit ) {
    Millisecond millisecond( getSecond() );
    millisecond += 500;
    EXPECT_EQ( millisecond.getUnit(), 500 );
    millisecond += 499;
    EXPECT_EQ( millisecond.getUnit(), 999 );
    millisecond += 1;
    EXPECT_EQ( millisecond.getUnit(), 0 );
    EXPECT_EQ( getSecond().getUnit(), 1 );
    millisecond += 1000;
    EXPECT_EQ( millisecond.getUnit(), 0 );
    EXPECT_EQ( getSecond().getUnit(), 2 );
    millisecond += 5000;
    EXPECT_EQ( millisecond.getUnit(), 0 );
    EXPECT_EQ( getSecond().getUnit(), 7 );
}

TEST_F( MillisecondTestSuite, substractUnit ) {
    Millisecond millisecond( getSecond() );
    millisecond -= 1;
    EXPECT_EQ( millisecond.getUnit(), 999 );
    EXPECT_EQ( getSecond().getUnit(), 59 );
    millisecond -= 500;
    EXPECT_EQ( millisecond.getUnit(), 499 );
    EXPECT_EQ( getSecond().getUnit(), 59 );
    millisecond -= 499;
    EXPECT_EQ( millisecond.getUnit(), 0 );
    EXPECT_EQ( getSecond().getUnit(), 59 );
    millisecond -= 1000;
    EXPECT_EQ( millisecond.getUnit(), 0 );
    EXPECT_EQ( getSecond().getUnit(), 58 );
    millisecond -= 5000;
    EXPECT_EQ( millisecond.getUnit(), 0 );
    EXPECT_EQ( getSecond().getUnit(), 53 );
}

TEST_F( MillisecondTestSuite, increment ) {
    Millisecond millisecond( getSecond() );
    ++millisecond;
    EXPECT_EQ( millisecond.getUnit(), 1 );
    ++millisecond;
    EXPECT_EQ( millisecond.getUnit(), 2 );
    ++millisecond;
    EXPECT_EQ( millisecond.getUnit(), 3 );
    millisecond += 996;
    EXPECT_EQ( millisecond.getUnit(), 999 );
    ++millisecond;
    EXPECT_EQ( millisecond.getUnit(), 0 );
    EXPECT_EQ( getSecond().getUnit(), 1 );
}

TEST_F( MillisecondTestSuite, decrement ) {
    Millisecond millisecond( getSecond() );
    --millisecond;
    EXPECT_EQ( millisecond.getUnit(), 999 );
    EXPECT_EQ( getSecond().getUnit(), 59 );
    --millisecond;
    EXPECT_EQ( millisecond.getUnit(), 998 );
    EXPECT_EQ( getSecond().getUnit(), 59 );
    --millisecond;
    EXPECT_EQ( millisecond.getUnit(), 997 );
    EXPECT_EQ( getSecond().getUnit(), 59 );
    millisecond -= 997;
    EXPECT_EQ( millisecond.getUnit(), 0 );
    --millisecond;
    EXPECT_EQ( millisecond.getUnit(), 999 );
    EXPECT_EQ( getSecond().getUnit(), 58 );
}

TEST_F( MillisecondTestSuite, toClockDuration ) {
    using namespace std::chrono;
    Millisecond millisecond( getSecond() );
    EXPECT_EQ( millisecond.toClockDuration(), milliseconds( 0 ) );
    ++millisecond;
    EXPECT_EQ( millisecond.toClockDuration(), milliseconds( 1 ) );
    ++millisecond;
    EXPECT_EQ( millisecond.toClockDuration(), milliseconds( 2 ) );
    millisecond += 997;
    EXPECT_EQ( millisecond.toClockDuration(), milliseconds( 999 ) );
    millisecond += 1;
    EXPECT_EQ( millisecond.toClockDuration(), milliseconds( 0 ) );
    millisecond += 500;
    EXPECT_EQ( millisecond.toClockDuration(), milliseconds( 500 ) );
    millisecond += 500;
    EXPECT_EQ( millisecond.toClockDuration(), seconds( 0 ) );
}

/*-------------------------------------------------------------------------------------------------------------------*/
TEST_F( DateTestSuite, testConvertion ) {
    TimePoint now = std::chrono::system_clock::now();
    ClockDuration roundedNow = now.time_since_epoch() / std::chrono::milliseconds( 1 ) * std::chrono::milliseconds( 1 );
    EXPECT_EQ( Date( now ).toClockDuration(), roundedNow );
    EXPECT_EQ( Date( TimePoint( ClockDuration( 1752384408000000000ll ) ) ).toClockDuration(),
               ClockDuration( 1752384408000000000ll ) );
}

TEST_F( DateTestSuite, testDate ) {
    Date date( TimePoint( ClockDuration( 1752384408000000000ll ) ) );
    EXPECT_EQ( date.getYear(), 2025 );
    EXPECT_EQ( date.getMonth(), 7 );
    EXPECT_EQ( date.getDay(), 13 );
    EXPECT_EQ( date.getHour(), 5 );
    EXPECT_EQ( date.getMinute(), 26 );
    EXPECT_EQ( date.getSecond(), 48 );
    EXPECT_EQ( date.getMillisecond(), 0 );
    EXPECT_EQ( date.getWeekday(), 7 );
}

TEST_F( DateTestSuite, addYears ) {
    Date date( TimePoint( ClockDuration( 1752384408000000000ll ) ) );
    auto prevWeekday = date.getWeekday();
    date.addYears( 25 );
    EXPECT_EQ( date.getYear(), 2050 );
    EXPECT_EQ( date.getMonth(), 7 );
    EXPECT_EQ( date.getDay(), 13 );
    EXPECT_EQ( date.getHour(), 5 );
    EXPECT_EQ( date.getMinute(), 26 );
    EXPECT_EQ( date.getSecond(), 48 );
    EXPECT_EQ( date.getMillisecond(), 0 );

    EXPECT_NE( date.getWeekday(), prevWeekday ) << date;
    EXPECT_EQ( date.getWeekday(), 3 ) << "Should be Wednesday: " << date;
}

TEST_F( DateTestSuite, addMonths ) {
    Date date( TimePoint( ClockDuration( 1752384408000000000ll ) ) );
    auto prevWeekday = date.getWeekday();
    EXPECT_EQ( date.getWeekday(), 7 );
    date.addMonths( 3 );
    EXPECT_EQ( date.getYear(), 2025 );
    EXPECT_EQ( date.getMonth(), 10 );
    EXPECT_EQ( date.getDay(), 13 );
    EXPECT_EQ( date.getHour(), 5 );
    EXPECT_EQ( date.getMinute(), 26 );
    EXPECT_EQ( date.getSecond(), 48 );
    EXPECT_EQ( date.getMillisecond(), 0 );
    EXPECT_EQ( date.getWeekday(), 1 );
    date.addMonths( 3 );
    EXPECT_EQ( date.getYear(), 2026 );
    EXPECT_EQ( date.getMonth(), 1 );
    EXPECT_EQ( date.getDay(), 13 );
    EXPECT_EQ( date.getHour(), 5 );
    EXPECT_EQ( date.getMinute(), 26 );
    EXPECT_EQ( date.getSecond(), 48 );
    EXPECT_EQ( date.getMillisecond(), 0 );
    EXPECT_EQ( date.getWeekday(), 2 );
}

TEST_F( DateTestSuite, addDays ) {
    Date date( TimePoint( ClockDuration( 1752384408000000000ll ) ) );
    date.addDays( 10 );
    EXPECT_EQ( date.getYear(), 2025 );
    EXPECT_EQ( date.getMonth(), 7 );
    EXPECT_EQ( date.getDay(), 23 );
    EXPECT_EQ( date.getHour(), 5 );
    EXPECT_EQ( date.getMinute(), 26 );
    EXPECT_EQ( date.getSecond(), 48 );
    EXPECT_EQ( date.getMillisecond(), 0 );
    date.addDays( 30 );
    EXPECT_EQ( date.getYear(), 2025 );
    EXPECT_EQ( date.getMonth(), 8 );
    EXPECT_EQ( date.getDay(), 22 );
    EXPECT_EQ( date.getHour(), 5 );
    EXPECT_EQ( date.getMinute(), 26 );
    EXPECT_EQ( date.getSecond(), 48 );
    EXPECT_EQ( date.getMillisecond(), 0 );
}

TEST_F( DateTestSuite, addHours ) {
    Date date( TimePoint( ClockDuration( 1752384408000000000ll ) ) );
    date.addHours( 10 );
    EXPECT_EQ( date.getYear(), 2025 );
    EXPECT_EQ( date.getMonth(), 7 );
    EXPECT_EQ( date.getDay(), 13 );
    EXPECT_EQ( date.getHour(), 15 );
    EXPECT_EQ( date.getMinute(), 26 );
    EXPECT_EQ( date.getSecond(), 48 );
    EXPECT_EQ( date.getMillisecond(), 0 );
    date.addHours( 30 );
    EXPECT_EQ( date.getYear(), 2025 );
    EXPECT_EQ( date.getMonth(), 7 );
    EXPECT_EQ( date.getDay(), 14 );
    EXPECT_EQ( date.getHour(), 21 );
    EXPECT_EQ( date.getMinute(), 26 );
    EXPECT_EQ( date.getSecond(), 48 );
    EXPECT_EQ( date.getMillisecond(), 0 );
}

TEST_F( DateTestSuite, addSeconds ) {
    {
        Date date( "2019-07-25T17:28:00+0000" );
        date.addSeconds( 5594 );
        EXPECT_EQ( date.toString(), "2019-07-25T19:01:14+0000" );
    }
    {
        Date date( "2019-07-25T17:28:00+0000" );
        date.addSeconds( -5594 );
        EXPECT_EQ( date.toString(), "2019-07-25T15:54:46+0000" );
    }
    {
        Date date( "2019-07-03T01:31:00+0000" );
        date.addSeconds( -5594 );
        EXPECT_EQ( date.toString(), "2019-07-02T23:57:46+0000" );
    }
}

TEST_F( DateTestSuite, incrementMonth ) {
    Date date( TimePoint( ClockDuration( 1738301208000000000ll ) ) );
    date.incrementMonth();
    EXPECT_EQ( date.getYear(), 2025 );
    EXPECT_EQ( date.getMonth(), 3 );
    EXPECT_EQ( date.getDay(), 31 );
    EXPECT_EQ( date.getHour(), 5 );
    EXPECT_EQ( date.getMinute(), 26 );
    EXPECT_EQ( date.getSecond(), 48 );
    EXPECT_EQ( date.getMillisecond(), 0 );
    date.incrementMonth();
    EXPECT_EQ( date.getYear(), 2025 );
    EXPECT_EQ( date.getMonth(), 5 );
    EXPECT_EQ( date.getDay(), 31 );
    EXPECT_EQ( date.getHour(), 5 );
    EXPECT_EQ( date.getMinute(), 26 );
    EXPECT_EQ( date.getSecond(), 48 );
    EXPECT_EQ( date.getMillisecond(), 0 );
}

TEST_F( DateTestSuite, decrementMonth ) {
    Date date( TimePoint( ClockDuration( 1738301208000000000ll ) ) );
    date.decrementMonth();
    EXPECT_EQ( date.getYear(), 2024 );
    EXPECT_EQ( date.getMonth(), 12 );
    EXPECT_EQ( date.getDay(), 31 );
    EXPECT_EQ( date.getHour(), 5 );
    EXPECT_EQ( date.getMinute(), 26 );
    EXPECT_EQ( date.getSecond(), 48 );
    EXPECT_EQ( date.getMillisecond(), 0 );
    date.decrementMonth();
    EXPECT_EQ( date.getYear(), 2024 );
    EXPECT_EQ( date.getMonth(), 10 );
    EXPECT_EQ( date.getDay(), 31 );
    EXPECT_EQ( date.getHour(), 5 );
    EXPECT_EQ( date.getMinute(), 26 );
    EXPECT_EQ( date.getSecond(), 48 );
    EXPECT_EQ( date.getMillisecond(), 0 );
}

TEST_F( DateTestSuite, incrementWeek ) {
    Date date( TimePoint( ClockDuration( 1738301208000000000ll ) ) );
    EXPECT_EQ( date.getWeekday(), 5 );
    date.incrementWeek();
    EXPECT_EQ( date.getYear(), 2025 );
    EXPECT_EQ( date.getMonth(), 2 );
    EXPECT_EQ( date.getDay(), 7 );
    EXPECT_EQ( date.getWeekday(), 5 );
    EXPECT_EQ( date.getHour(), 5 );
    EXPECT_EQ( date.getMinute(), 26 );
    EXPECT_EQ( date.getSecond(), 48 );
    EXPECT_EQ( date.getMillisecond(), 0 );
    date.incrementWeek();
    EXPECT_EQ( date.getYear(), 2025 );
    EXPECT_EQ( date.getMonth(), 2 );
    EXPECT_EQ( date.getDay(), 14 );
    EXPECT_EQ( date.getWeekday(), 5 );
    EXPECT_EQ( date.getHour(), 5 );
    EXPECT_EQ( date.getMinute(), 26 );
    EXPECT_EQ( date.getSecond(), 48 );
    EXPECT_EQ( date.getMillisecond(), 0 );
}

TEST_F( DateTestSuite, decrementWeek ) {
    Date date( TimePoint( ClockDuration( 1738301208000000000ll ) ) );
    date.decrementWeek();
    EXPECT_EQ( date.getYear(), 2025 );
    EXPECT_EQ( date.getMonth(), 1 );
    EXPECT_EQ( date.getDay(), 24 );
    EXPECT_EQ( date.getWeekday(), 5 );
    EXPECT_EQ( date.getHour(), 5 );
    EXPECT_EQ( date.getMinute(), 26 );
    EXPECT_EQ( date.getSecond(), 48 );
    EXPECT_EQ( date.getMillisecond(), 0 );
    date.decrementWeek();
    EXPECT_EQ( date.getYear(), 2025 );
    EXPECT_EQ( date.getMonth(), 1 );
    EXPECT_EQ( date.getDay(), 17 );
    EXPECT_EQ( date.getWeekday(), 5 );
    EXPECT_EQ( date.getHour(), 5 );
    EXPECT_EQ( date.getMinute(), 26 );
    EXPECT_EQ( date.getSecond(), 48 );
    EXPECT_EQ( date.getMillisecond(), 0 );
    date.decrementWeek();
    date.decrementWeek();
    date.decrementWeek();
    EXPECT_EQ( date.getYear(), 2024 );
    EXPECT_EQ( date.getMonth(), 12 );
    EXPECT_EQ( date.getDay(), 27 );
    EXPECT_EQ( date.getWeekday(), 5 );
    EXPECT_EQ( date.getHour(), 5 );
    EXPECT_EQ( date.getMinute(), 26 );
    EXPECT_EQ( date.getSecond(), 48 );
    EXPECT_EQ( date.getMillisecond(), 0 );
}

TEST_F( DateTestSuite, timeZoneOffset ) {
    auto unixTime = ClockDuration( 1738301208000000000ll );
    Date date = Date( TimePoint( unixTime ), std::chrono::seconds( 2 * 60 * 60 ) );
    LOGGER_TRACE_RAW( toStandardString( date ) );
    EXPECT_EQ( date.toClockDuration(), unixTime );
    EXPECT_EQ( date.getYear(), 2025 );
    EXPECT_EQ( date.getMonth(), 1 );
    EXPECT_EQ( date.getDay(), 31 );
    EXPECT_EQ( date.getHour(), 7 );
    EXPECT_EQ( date.getMinute(), 26 );
    EXPECT_EQ( date.getSecond(), 48 );
    EXPECT_EQ( date.getMillisecond(), 0 );

    date = Date( TimePoint( unixTime ), std::chrono::seconds( -2 * 60 * 60 ) );
    LOGGER_TRACE_RAW( toStandardString( date ) );
    EXPECT_EQ( date.toClockDuration(), unixTime );
    EXPECT_EQ( date.getYear(), 2025 );
    EXPECT_EQ( date.getMonth(), 1 );
    EXPECT_EQ( date.getDay(), 31 );
    EXPECT_EQ( date.getHour(), 3 );
    EXPECT_EQ( date.getMinute(), 26 );
    EXPECT_EQ( date.getSecond(), 48 );
    EXPECT_EQ( date.getMillisecond(), 0 );

    date = Date( TimePoint( unixTime ), std::chrono::seconds( -6 * 60 * 60 ) );
    LOGGER_TRACE_RAW( toStandardString( date ) );
    EXPECT_EQ( date.toClockDuration(), unixTime );
    EXPECT_EQ( date.getYear(), 2025 );
    EXPECT_EQ( date.getMonth(), 1 );
    EXPECT_EQ( date.getDay(), 30 );
    EXPECT_EQ( date.getHour(), 23 );
    EXPECT_EQ( date.getMinute(), 26 );
    EXPECT_EQ( date.getSecond(), 48 );
    EXPECT_EQ( date.getMillisecond(), 0 );

    date = Date( TimePoint( unixTime ), std::chrono::seconds( 24 * 60 * 60 ) );
    LOGGER_TRACE_RAW( toStandardString( date ) );
    EXPECT_EQ( date.toClockDuration(), unixTime );
    EXPECT_EQ( date.getYear(), 2025 );
    EXPECT_EQ( date.getMonth(), 2 );
    EXPECT_EQ( date.getDay(), 1 );
    EXPECT_EQ( date.getHour(), 5 );
    EXPECT_EQ( date.getMinute(), 26 );
    EXPECT_EQ( date.getSecond(), 48 );
    EXPECT_EQ( date.getMillisecond(), 0 );
}

TEST_F( DateTestSuite, toString ) {
    {
        std::string dateString = "2019-01-05:10:19:58";
        GregorianCalendar::Date date( dateString, "%Y-%m-%d:%H:%M:%S" );
        EXPECT_EQ( dateString, date.toString( "%Y-%m-%d:%H:%M:%S" ) );
    }
    {
        std::string dateString = "2019-02-20T23:10:34+0200";
        GregorianCalendar::Date date( dateString );
        EXPECT_EQ( dateString, date.toString() );
        EXPECT_EQ( 1550697034000000000LL, date.toClockDuration().count() );
    }
    {
        std::string dateString = "2019-02-20T23:10:34+0000";
        GregorianCalendar::Date date( dateString );
        EXPECT_EQ( dateString, date.toString() );
        EXPECT_EQ( 1550704234000000000LL, date.toClockDuration().count() );
    }
    {
        std::string dateString = "2019-12-31T07:33:00+0000";
        GregorianCalendar::Date date( dateString );
        EXPECT_EQ( dateString, date.toString() );
    }
    {
        std::string dateString = "2019-12-31T07:33:00";
        GregorianCalendar::Date date( dateString );
        EXPECT_EQ( dateString, date.toString( "%FT%T" ) );
    }
}

TEST_F( DateTestSuite, testDecember31 ) {
    std::string dateString = "2019-12-31T07:33:00";
    Date date( TimePoint( ClockDuration( 1577777580000000000LL ) ) );
    EXPECT_EQ( dateString, date.toString( "%FT%T" ) );
}

TEST_F( DateTestSuite, testNewYearDayIncrement ) {
    {
        GregorianCalendar::Date date( "2020-12-31T15:43:00" );
        date.incrementDay();
        EXPECT_EQ( 1609515780000000000LL, date.toClockDuration().count() );
    }
    {
        GregorianCalendar::Date date( "2021-12-31T15:43:00" );
        date.incrementDay();
        EXPECT_EQ( 1641051780000000000LL, date.toClockDuration().count() );
    }
    {
        GregorianCalendar::Date date( "2022-12-31T15:43:00" );
        date.incrementDay();
        EXPECT_EQ( 1672587780000000000LL, date.toClockDuration().count() );
    }
    {
        GregorianCalendar::Date date( "2023-12-31T15:43:00" );
        date.incrementDay();
        EXPECT_EQ( 1704123780000000000LL, date.toClockDuration().count() );
    }
    {
        GregorianCalendar::Date date( "2024-12-31T15:43:00" );
        date.incrementDay();
        EXPECT_EQ( 1735746180000000000LL, date.toClockDuration().count() );
    }
    {
        GregorianCalendar::Date date( "2025-12-31T15:43:00" );
        date.incrementDay();
        EXPECT_EQ( 1767282180000000000LL, date.toClockDuration().count() );
    }
    {
        GregorianCalendar::Date date( "2026-12-31T15:43:00" );
        date.incrementDay();
        EXPECT_EQ( 1798818180000000000LL, date.toClockDuration().count() );
    }
}

TEST_F( DateTestSuite, comparisonOperators ) {
    const std::string dateString1 = "2019-12-31T07:33:00";
    const std::string dateString2 = "2020-12-31T17:55:00";
    Date date1( dateString1 );
    Date date2( dateString2 );
    EXPECT_FALSE( date1 == date2 );
    EXPECT_TRUE( date1 != date2 );
    EXPECT_TRUE( date1 < date2 );
    EXPECT_TRUE( date1 <= date2 );
    EXPECT_FALSE( date1 > date2 );
    EXPECT_FALSE( date1 >= date2 );

    EXPECT_FALSE( date2 == date1 );
    EXPECT_TRUE( date2 != date1 );
    EXPECT_FALSE( date2 < date1 );
    EXPECT_FALSE( date2 <= date1 );
    EXPECT_TRUE( date2 > date1 );
    EXPECT_TRUE( date2 >= date1 );
}

TEST_F( DateTestSuite, setDate ) {
    Date date;
    ASSERT_TRUE( date.setDate( 2013, 2, 21 ) );
    EXPECT_EQ( date.toString(), "2013-02-21T00:00:00+0000" );

    ASSERT_FALSE( date.setDate( 2013, 0, 21 ) );
    ASSERT_FALSE( date.setDate( 2013, 13, 21 ) );
    ASSERT_FALSE( date.setDate( 2013, 1, 0 ) );
    ASSERT_FALSE( date.setDate( 2013, 1, 35 ) );
}

TEST_F( DateTestSuite, setTime ) {
    Date date;
    date.setYear( 2021 );
    ASSERT_TRUE( date.setTime( 12, 34, 45 ) );
    EXPECT_EQ( date.toString(), "2021-01-01T12:34:45+0000" );

    ASSERT_FALSE( date.setTime( 25, 34, 45 ) );
    ASSERT_FALSE( date.setTime( 12, 65, 45 ) );
    ASSERT_FALSE( date.setTime( 12, 34, 100 ) );
}

TEST_F( DateTestSuite, daysFromYearBeginning ) {
    Date date;

    ASSERT_TRUE( date.setDate( 2019, 12, 31 ) );
    ASSERT_EQ( date.getDaysFromYearBeginning().count(), 365 );

    ASSERT_TRUE( date.setDate( 2020, 12, 31 ) );
    ASSERT_EQ( date.getDaysFromYearBeginning().count(), 366 );
}

class WeeksTestSuite : public ::testing::Test {};

TEST_F( WeeksTestSuite, weekOfYearCountedFromOne_weekStartFromMonday ) {
    Date date;
    ASSERT_TRUE( date.setDate( 2024, 1, 1 ) );
    ASSERT_EQ( (int) date.getWeekday(), 1 );
    ASSERT_EQ( (int) getWeekOfYear( date ), 1 );
}

TEST_F( WeeksTestSuite, weekOfYearCountedFromOne_weekStartFromOtherWeekday ) {
    Date date;
    ASSERT_TRUE( date.setDate( 2022, 1, 1 ) );
    ASSERT_EQ( (int) date.getWeekday(), 6 );
    ASSERT_EQ( (int) getWeekOfYear( date ), 1 );
}

TEST_F( WeeksTestSuite, weekOfYear ) {
    Date date;

    EXPECT_TRUE( date.setDate( 2021, 1, 1 ) );
    EXPECT_EQ( (int) getWeekOfYear( date ), 1 ) << date;

    EXPECT_TRUE( date.setDate( 2021, 1, 8 ) );
    EXPECT_EQ( (int) getWeekOfYear( date ), 2 ) << date;

    EXPECT_TRUE( date.setDate( 2021, 1, 15 ) );
    EXPECT_EQ( (int) getWeekOfYear( date ), 3 ) << date;

    EXPECT_TRUE( date.setDate( 2021, 1, 22 ) );
    EXPECT_EQ( (int) getWeekOfYear( date ), 4 ) << date;

    EXPECT_TRUE( date.setDate( 2021, 1, 29 ) );
    EXPECT_EQ( (int) getWeekOfYear( date ), 5 ) << date;

    EXPECT_TRUE( date.setDate( 2021, 1, 31 ) );
    EXPECT_EQ( (int) getWeekOfYear( date ), 5 ) << date;

    EXPECT_TRUE( date.setDate( 2021, 12, 31 ) );
    EXPECT_EQ( (int) getWeekOfYear( date ), 53 ) << date;

    EXPECT_TRUE( date.setDate( 2020, 12, 31 ) );
    EXPECT_EQ( (int) getWeekOfYear( date ), 53 ) << date;

    EXPECT_TRUE( date.setDate( 2021, 1, 3 ) );
    EXPECT_EQ( (int) getWeekOfYear( date ), 1 ) << date;
    EXPECT_TRUE( date.setDate( 2021, 1, 4 ) );
    EXPECT_EQ( (int) getWeekOfYear( date ), 2 ) << date;
    EXPECT_TRUE( date.setDate( 2021, 1, 11 ) );
    EXPECT_EQ( (int) getWeekOfYear( date ), 3 ) << date;
}

TEST_F( WeeksTestSuite, weekOfMonth ) {
    Date date;

    EXPECT_TRUE( date.setDate( 2021, 1, 1 ) );
    EXPECT_EQ( (int) getWeekOfMonth( date ), 1 ) << date;

    EXPECT_TRUE( date.setDate( 2021, 1, 8 ) );
    EXPECT_EQ( (int) getWeekOfMonth( date ), 2 ) << date;

    EXPECT_TRUE( date.setDate( 2021, 1, 15 ) );
    EXPECT_EQ( (int) getWeekOfMonth( date ), 3 ) << date;

    EXPECT_TRUE( date.setDate( 2021, 1, 22 ) );
    EXPECT_EQ( (int) getWeekOfMonth( date ), 4 ) << date;

    EXPECT_TRUE( date.setDate( 2021, 1, 29 ) );
    EXPECT_EQ( (int) getWeekOfMonth( date ), 5 ) << date;

    EXPECT_TRUE( date.setDate( 2021, 1, 31 ) );
    EXPECT_EQ( (int) getWeekOfMonth( date ), 5 ) << date;

    EXPECT_TRUE( date.setDate( 2021, 5, 31 ) );
    EXPECT_EQ( (int) getWeekOfMonth( date ), 6 ) << date;

    EXPECT_TRUE( date.setDate( 2021, 8, 30 ) );
    EXPECT_EQ( (int) getWeekOfMonth( date ), 6 ) << date;

    EXPECT_TRUE( date.setDate( 2021, 11, 30 ) );
    EXPECT_EQ( (int) getWeekOfMonth( date ), 5 ) << date;

    EXPECT_TRUE( date.setDate( 2020, 12, 31 ) );
    EXPECT_EQ( (int) getWeekOfMonth( date ), 5 ) << date;
}

TEST_F( WeeksTestSuite, weekOfMonthMatchWeekOfYear ) {
    Date date;
    for( int64_t year = 1970; year < 2200; ++year ) {
        for( uint8_t day = 1; day <= 31; ++day ) {
            date.setDate( year, 1, day );
            EXPECT_EQ( getWeekOfMonth( date ), getWeekOfYear( date ) ) << date;
        }
    }
}

TEST_F( WeeksTestSuite, lastWeekOfYearNumber ) {
    Date yearEnding;
    Date yearBeginning;
    for( int64_t year = 1970; year < 2200; ++year ) {
        yearBeginning.setDate( year, 1, 1 );
        yearEnding.setDate( year, 12, 31 );
        yearEnding.setTime( 23, 59, 59 );

        const bool isYearWithMaxWeeks = yearBeginning.getWeekday() == 7 && Year( year ).isLeapYear();
        const int expectedWeeksInYear = isYearWithMaxWeeks ? 54 : 53;
        EXPECT_EQ( (int) getWeekOfYear( yearEnding ), expectedWeeksInYear ) << yearEnding;
    }
}

TEST_F( DateTestSuite, weekdaysHandling ) {
    // default ctor, doesn't set weekdays
    Date date;
    // set to 2021-08-27, Friday (weekday=5)
    date.setYear( 2021 );
    date.setMonth( 8 );
    date.setDay( 27 );
    LOGGER_TRACE_RAW( toStandardString( date ) );
    const int Thursday = 4, Friday = 5;
    EXPECT_EQ( date.getWeekday(), Friday ) << date;

    date.decrementMillisecond();
    LOGGER_TRACE_RAW( toStandardString( date ) );
    EXPECT_EQ( date.getWeekday(), Thursday ) << date;
    date.incrementMillisecond();
    EXPECT_EQ( date.getWeekday(), Friday ) << date;

    date.decrementSecond();
    LOGGER_TRACE_RAW( toStandardString( date ) );
    EXPECT_EQ( date.getWeekday(), Thursday ) << date;
    date.incrementSecond();
    EXPECT_EQ( date.getWeekday(), Friday ) << date;

    date.decrementMinute();
    LOGGER_TRACE_RAW( toStandardString( date ) );
    EXPECT_EQ( date.getWeekday(), Thursday ) << date;
    date.incrementMinute();
    EXPECT_EQ( date.getWeekday(), Friday ) << date;

    date.decrementHour();
    LOGGER_TRACE_RAW( toStandardString( date ) );
    EXPECT_EQ( date.getWeekday(), Thursday ) << date;
    date.incrementHour();
    EXPECT_EQ( date.getWeekday(), Friday ) << date;
}

TEST_F( DateTestSuite, fromUtcTime ) {
    using namespace std::chrono;

    auto timePointUtc = system_clock::now();
    auto tzOffset = hours( 3 );

    LOGGER_DEBUG( "1: Create Date from system_clock::now() / utc timepoint" );

    Date utcNow( timePointUtc );
    // FIXME: %Z shows timezone from environment (EEST for +0300 Kiev/Ukraine), not UTC: %Z can't be used to utc time!
    LOGGER_DEBUG( "utcNow.toString: " + utcNow.toString( "%FT%T%z %Z" ) );
    EXPECT_EQ( utcNow.toString( "%z" ), "+0000" );

    LOGGER_DEBUG( "2: Create Date from system_clock::now() / utc timepoint AND positive timezone offset" );
    auto timePointLocal = timePointUtc + tzOffset;
    Date localNow = Date::fromLocalTime( timePointLocal, tzOffset );

    LOGGER_DEBUG( "localNow.toString: " + localNow.toString( "%FT%T%z %Z" ) );

    auto clockDuration = localNow.toClockDuration();
    auto clockTimePoint = localNow.toTimePoint();
    auto tz = localNow.getTimeZoneOffset();

    LOGGER_DEBUG( "3:  Create Date from localNow (clockDuration, tz)" );

    Date localFromDuration = Date::fromUtcTime( clockDuration, tz );
    LOGGER_DEBUG( "localNow: " + localNow.toString() );
    LOGGER_DEBUG( "localFromDuration: " + localFromDuration.toString() );

    EXPECT_EQ( localNow.toString(), localFromDuration.toString() ) << "Local calendar date should match";
    EXPECT_EQ( localNow.toString( "%z" ), localFromDuration.toString( "%z" ) ) << "Timezone offset should match";

    EXPECT_EQ( localNow.toTimePoint(), localFromDuration.toTimePoint() ) << "UTC timestamps of both dates should match";
}

namespace {
std::chrono::seconds timeZoneOffset() {
    std::time_t current_time;
    std::time( &current_time );
    struct std::tm* timeinfo = std::localtime( &current_time );
    return std::chrono::seconds( timeinfo->tm_gmtoff );
}

std::chrono::seconds timeZoneOffset( std::chrono::system_clock::time_point timePoint ) {
    auto timestamp = std::chrono::system_clock::to_time_t( timePoint );
    std::tm timeInfo = *std::localtime( &timestamp );

    timeInfo.tm_isdst = -1;
    std::mktime( &timeInfo );
    return std::chrono::seconds( timeInfo.tm_gmtoff );
}
}  // namespace

TEST_F( DateTestSuite, fromLocalTime ) {
    Log::setLocalLogLevel( ( Log::TRACE ) );
    using namespace std::chrono;

    auto originalTZ = getenv( "TZ" );
    LOGGER_DEBUG( "original TZ: " + std::string( originalTZ ? originalTZ : "empty" ) );

    // This works. Make another timezone with different offset, Then get system_clock::now().
    EXPECT_EQ( setenv( "TZ", "Europe/Paris", 1 ), 0 );
    LOGGER_DEBUG( "Europa/Paris" );

    auto timePointUtc = system_clock::now();
    auto tzOffset = timeZoneOffset( timePointUtc );

    if( tzOffset.count() == 0 ) {
        LOGGER_ERROR( "Timezone offset is zero, test won't work" );
    }
    Date dateUtcWithOffset( timePointUtc, tzOffset );
    LOGGER_DEBUG( "dateUtc.toString: " + dateUtcWithOffset.toString() );

    const char* const TimePointFormatFT = "%FT%T";
    const char* const TimePointFormatFTz = "%FT%T%z";
    EXPECT_EQ( dateUtcWithOffset.toString( TimePointFormatFTz ),
               TimeUtils::chronoTimePointToString( timePointUtc, TimePointFormatFTz ) )
        << "Date/Time formatted corresponds to local time";

    // clock.h: localTime() -> greenwichMeanTimePoint() + timeZoneOffset()
    auto timePointLocal = timePointUtc + tzOffset;

    {
        LOGGER_DEBUG( "1: Create Date from system_clock::now() + offset / local timepoint" );

        Date dateFromLocalTimePointNoTzOffset( timePointLocal );
        // YYYY-mm-dd HH:MM:SS - corresponds to local time, but offset 0000
        LOGGER_DEBUG( "dateFromLocalTimePointNoTzOffset.toString: " +
                      dateFromLocalTimePointNoTzOffset.toString( "%FT%T%z %Z" ) );

        EXPECT_NE( dateFromLocalTimePointNoTzOffset.toString( TimePointFormatFTz ),
                   TimeUtils::chronoTimePointToString( timePointUtc, TimePointFormatFTz ) )
            << "Local Date/Time formatted corresponds to local time, differs with offset only";
        EXPECT_EQ( dateFromLocalTimePointNoTzOffset.toString( TimePointFormatFT ),
                   TimeUtils::chronoTimePointToString( timePointUtc, TimePointFormatFT ) )
            << "Local Date/Time formatted corresponds to local time";
        EXPECT_EQ( dateFromLocalTimePointNoTzOffset.getTimeZoneOffset(), seconds( 0 ) ) << "No timezone offset";
    }

    /* E.6 How to use local time, offset:
     * - 1. Create a Date object with fromLocalTime( localNow, offset ) -> ;
     * - 2. can manipulate with dates as if Date objects contains local time calendar values;
     * - 3. Make calculations over local time
     * - 4. Return timepoint via Date::toDuration() -> returns UTC time point;
     * - 5. Create Date object from this (duration, offset) pair with Date::fromUtcTime( duration, offset ).
     * */
    {
        LOGGER_DEBUG( "2: Create with Date::fromLocalTime() from system_clock::now() + tzOffset / local timepoint AND "
                      "positive timezone offset" );
        Date dateFromLocalTimeWithTzOffset = Date::fromLocalTime( timePointLocal, tzOffset );
        LOGGER_DEBUG( "dateFromLocalTimeWithTzOffset.toString: " +
                      dateFromLocalTimeWithTzOffset.toString( "%FT%T%z %Z" ) );

        auto clockDuration = dateFromLocalTimeWithTzOffset.toClockDuration();
        auto tz = dateFromLocalTimeWithTzOffset.getTimeZoneOffset();

        LOGGER_DEBUG( "3:  Create Date from localNow (clockDuration, tz)" );

        Date localFromDuration = Date::fromUtcTime( clockDuration, tz );
        LOGGER_DEBUG( "dateFromLocalTimeWithTzOffset: " + dateFromLocalTimeWithTzOffset.toString() );
        LOGGER_DEBUG( "localFromDuration: " + localFromDuration.toString() );

        EXPECT_EQ( dateFromLocalTimeWithTzOffset.toString(), localFromDuration.toString() )
            << "Local calendar date should match";
        EXPECT_EQ( dateFromLocalTimeWithTzOffset.toString( "%z" ), localFromDuration.toString( "%z" ) )
            << "Timezone offset should match";
    }

    if( !originalTZ || !*originalTZ ) {
        unsetenv( "TZ" );
    }
}

}  // namespace UT
}  // namespace HubPlatform
