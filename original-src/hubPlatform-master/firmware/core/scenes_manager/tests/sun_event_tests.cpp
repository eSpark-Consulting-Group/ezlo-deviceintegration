#include "scenes_manager/scenes_engine/events/sun_state_event.h"
#include "scenes_manager/scenes_engine/helpers/dst.h"
#include "scenes_manager/tests/resources/sun_state_reference.h"
#include "base_utils/timeutils.h"
#include "common_utils/cpp14.h"
#include "scenes_manager/scenes_engine/helpers/test_clock.h"
#include "scenes_manager/scenes_engine/helpers/clock.h"
#include "log_utils/ezlo_log.h"

#include <gtest/gtest.h>

#include <ctime>
#include <sstream>

namespace SceneEngine {
namespace UT {

using namespace HubPlatform;

class SunStateEventTest : public testing::Test {
public:
    static std::string getSunriseDate( const TestData& data ) {
        std::stringstream ss;
        ss << data.date << "T" << data.sunrise;
        const int targetOffset = data.timeZoneOffset + data.dstChange;
        ss << ( targetOffset < 0 ? "-" : "+" );
        ss << std::setfill( '0' ) << std::setw( 2 ) << abs( targetOffset ) << "00";
        return ss.str();
    }

    static std::string getSunsetDate( const TestData& data ) {
        std::stringstream ss;
        ss << data.date << "T" << data.sunset;
        const int targetOffset = data.timeZoneOffset + data.dstChange;
        ss << ( targetOffset < 0 ? '-' : '+' );
        ss << std::setw( 2 ) << std::setfill( '0' ) << abs( targetOffset ) << "00";
        if( ss.str() == "2019-07-15T24:12:00-0700" ) {
            return "2019-07-16T00:12:00-0700";
        }
        return ss.str();
    }

    static std::string getMidnightDate( const TestData& data ) {
        std::stringstream ss;
        ss << data.date << "T00:00:00";
        return ss.str();
    }

    static std::size_t getTimeInterval( const std::tm& time ) {
        return time.tm_hour * 60 * 60 + time.tm_min * 60 + time.tm_sec;
    }

    void setSunState( const SunStateEvent::State& state ) {
        m_sunState = state;
    }

    void setDays( SunStateEvent::Days&& days ) {
        m_days = std::move( days );
    }

    void setWeekdays( SunStateEvent::Weekdays&& weekdays ) {
        m_weekdays = std::move( weekdays );
    }

    void copySunstateDate( const GregorianCalendar::Date& src, TestData& dst, SunStateEvent::State state ) {
        dst.date.year = src.getYear();
        dst.date.month = src.getMonth();
        dst.date.day = src.getDay();
        if( state == SunStateEvent::State::SUN_RISE ) {
            dst.sunrise.hours = src.getHour();
            dst.sunrise.minutes = src.getMinute();
            dst.sunrise.seconds = src.getSecond();
        }
        else {
            dst.sunset.hours = src.getHour();
            dst.sunset.minutes = src.getMinute();
            dst.sunset.seconds = src.getSecond();
        }
    }

    void addTimeOffset( TestData& data,
                        SunStateEvent::State state,
                        SunStateEvent::OffsetType offsetType,
                        const std::string& timeOffset ) {
        if( offsetType == SunStateEvent::OffsetType::IN_TIME ) {
            return;
        }
        struct tm timeOffsetStruct = TimeUtils::stringToTimeStruct( timeOffset, "%H:%M:%S" );
        std::string sunstateDate =
            ( state == SunStateEvent::State::SUN_RISE ) ? getSunriseDate( data ) : getSunsetDate( data );
        GregorianCalendar::Date date( sunstateDate );
        if( offsetType == SunStateEvent::OffsetType::AFTER ) {
            date.addSeconds( timeOffsetStruct.tm_sec );
            date.addMinutes( timeOffsetStruct.tm_min );
            date.addHours( timeOffsetStruct.tm_hour );
        }
        else if( offsetType == SunStateEvent::OffsetType::BEFORE ) {
            date.addSeconds( -timeOffsetStruct.tm_sec );
            date.addMinutes( -timeOffsetStruct.tm_min );
            date.addHours( -timeOffsetStruct.tm_hour );
        }
        copySunstateDate( date, data, state );
    }

    void testSunState( const std::vector< TestData >& sunStatesList,
                       SunStateEvent::OffsetType offsetType,
                       const JsonObject& timeOffset,
                       int codeLine ) {
        std::string codeLineStr = failedAt( codeLine );
        for( TestData data : sunStatesList ) {
            GregorianCalendar::Date currentDate( getMidnightDate( data ) );
            auto geoPositionGetter = [&data]() { return data.geoposition; };
            auto clockGetter = makeClockGetter( getMidnightDate( data ), std::chrono::hours( data.timeZoneOffset ) );
            SunStateEvent sunStateEvent( m_sunState, offsetType, geoPositionGetter, clockGetter );
            if( timeOffset.empty() ) {
                sunStateEvent.setTimeOffset( timeOffset );
                addTimeOffset( data, m_sunState, offsetType, timeOffset );
            }
            SunStateEvent::OptionalClock nextSunStateLocalTimePointPtr =
                sunStateEvent.getNextDate( GregorianCalendar::TimePoint( currentDate.toClockDuration() ),
                                           std::chrono::hours( data.timeZoneOffset ) );
            bool isSunStatePossible =
                ( m_sunState == SunStateEvent::State::SUN_RISE ) ? data.sunrise.hours == NaN : data.sunset.hours == NaN;
            if( isSunStatePossible ) {
                EXPECT_FALSE( nextSunStateLocalTimePointPtr ) << codeLineStr;
                continue;
            }
            else {
                EXPECT_TRUE( nextSunStateLocalTimePointPtr ) << codeLineStr;
            }
            auto sunStateLocalTimePoint = GregorianCalendar::TimePoint( nextSunStateLocalTimePointPtr->duration() );
            auto offset = nextSunStateLocalTimePointPtr->offset();
            std::string expectedDate =
                ( m_sunState == SunStateEvent::State::SUN_RISE ) ? getSunriseDate( data ) : getSunsetDate( data );
            std::string calculatedDate =
                GregorianCalendar::Date::fromLocalTime( sunStateLocalTimePoint, offset ).toString( "%FT%T%z" );
            EXPECT_EQ( calculatedDate, expectedDate ) << codeLineStr;
        }
    }

    void testSunState( SunStateEvent::OffsetType offsetType, const JsonObject& timeOffset, int codeLine ) {
        testSunState( ReferenceMonthlySunstatesList, offsetType, timeOffset, codeLine );
    }

    void runTestCases( const std::vector< TestData >& sunStatesList, SunStateEvent::State state ) {
        Log::setLocalLogLevel( Log::TRACE );
        setSunState( state );
        testSunState( sunStatesList, SunStateEvent::OffsetType::IN_TIME, "", __LINE__ );
        testSunState( sunStatesList, SunStateEvent::OffsetType::IN_TIME, "00:00:20", __LINE__ );
        testSunState( sunStatesList, SunStateEvent::OffsetType::AFTER, "12:40:30", __LINE__ );
        testSunState( sunStatesList, SunStateEvent::OffsetType::AFTER, "25:40:30", __LINE__ );
        testSunState( sunStatesList, SunStateEvent::OffsetType::BEFORE, "12:40:30", __LINE__ );
        testSunState( sunStatesList, SunStateEvent::OffsetType::BEFORE, "24:40:30", __LINE__ );
    }

    void testWeeklySunState( SunStateEvent::State state, int codeLine ) {
        std::string codeLineStr = failedAt( codeLine );
        using namespace std::chrono;
        using Weekday = SunStateEvent::Weekday;
        std::vector< int > weekdays = { (int) Weekday::THU, (int) Weekday::SUN, (int) Weekday::MON };
        SunStateEvent sunstateEvent( state,
                                     SunStateEvent::OffsetType::IN_TIME,
                                     []() {
                                         return HubPlatform::CoreDRemoteInterface::GeoPosition{ 49.99450, 36.21506 };
                                     },
                                     makeClockGetter( "", hours( 3 ) ) );

        sunstateEvent.setWeekdays( weekdays );
        std::sort( weekdays.begin(), weekdays.end() );
        GregorianCalendar::Date weekdayIterator( "2019-09-02T00:00:00+0300", "%FT%T%z" );
        const int repeatingTimes = 4;
        for( int i = 0; i < repeatingTimes; ++i ) {
            for( const int& weekday : weekdays ) {
                weekdayIterator.moveToWeekday( weekday - 1 );
                SunStateEvent::OptionalClock localTimePoint = sunstateEvent.getNextDate(
                    GregorianCalendar::TimePoint( weekdayIterator.toClockDuration() ), hours( 3 ) );
                EXPECT_TRUE( localTimePoint ) << codeLineStr;
                if( localTimePoint ) {
                    auto suntateTimePoint = fromLocalTimePoint( *localTimePoint );
                    const TestData& referenceSunstate = ReferenceWeeklySunstatesList[ suntateTimePoint.getDay() - 1 ];
                    std::string sunstateDate = ( state == SunStateEvent::State::SUN_RISE ) ?
                                                   getSunriseDate( referenceSunstate ) :
                                                   getSunsetDate( referenceSunstate );
                    EXPECT_EQ( suntateTimePoint.toString( "%FT%T%z" ), sunstateDate ) << codeLineStr;
                }
            }
            weekdayIterator.addWeekdays( 2 );
        }
    }

    void testMonthlySunState( SunStateEvent::State state, int codeLine ) {
        std::string codeLineStr = failedAt( codeLine );
        using namespace std::chrono;
        SunStateEvent::Days days = { 28, 5, 1, 11, 25, 17 };
        SunStateEvent sunstateEvent( state,
                                     SunStateEvent::OffsetType::IN_TIME,
                                     []() {
                                         return HubPlatform::CoreDRemoteInterface::GeoPosition{ 49.99450, 36.21506 };
                                     },
                                     makeClockGetter( "", hours( 3 ) ) );
        sunstateEvent.setDays( days );
        std::sort( days.begin(), days.end() );
        GregorianCalendar::Date dayIterator( "2019-07-31T00:00:00", "%FT%T" );
        const int repeatingTimes = 3;
        int counter = 0;
        for( int i = 0; i < repeatingTimes; ++i ) {
            int oldDay = -1;
            for( const SunStateEvent::Day& day : days ) {
                SunStateEvent::OptionalClock localTimePoint = sunstateEvent.getNextDate(
                    GregorianCalendar::TimePoint( dayIterator.toClockDuration() ), hours( 3 ) );
                dayIterator.addDays( day - oldDay );
                oldDay = day;
                EXPECT_TRUE( localTimePoint ) << codeLineStr;
                if( localTimePoint ) {
                    auto suntateTimePoint = fromLocalTimePoint( *localTimePoint );
                    const TestData& referenceSunstate = ReferenceMonthlySunstatesList[ counter ];
                    std::string expectedDateString = ( state == SunStateEvent::State::SUN_RISE ) ?
                                                         getSunriseDate( referenceSunstate ) :
                                                         getSunsetDate( referenceSunstate );

                    auto sunstateString = suntateTimePoint.toString( "%FT%T%z" );
                    auto duration = suntateTimePoint.toClockDuration();
                    GregorianCalendar::Date expectedDate( expectedDateString, "%FT%T%z" );
                    auto expectedDuration = expectedDate.toClockDuration();
                    // getSunriseDate/getSunsetDate actually don't switch timezone offset, use durations
                    EXPECT_EQ( duration, expectedDuration )
                        << "UTC marks should match: durations differ for: " << sunstateString << ", "
                        << expectedDateString;
                }
                ++counter;
            }
            dayIterator.setDay( dayIterator.getMaxMonthDay() );
        }
    }

    SunStateEvent::ClockGetter makeClockGetter( const std::string& testDate, std::chrono::hours timeZoneOffsetHours ) {
        if( !testDate.empty() ) {
            using namespace GregorianCalendar;
            Date date( testDate );
            m_clockPtr = std::make_unique< TestClock >( date.toSeconds(), timeZoneOffsetHours );
        }
        else {
            m_clockPtr = std::make_unique< TestClock >( std::chrono::duration_cast< std::chrono::seconds >(
                                                            std::chrono::system_clock::now().time_since_epoch() ),
                                                        timeZoneOffsetHours );
        }
        auto clockGetter = [this]() mutable -> IClock& { return *m_clockPtr; };
        return clockGetter;
    }

    GregorianCalendar::Date fromLocalTimePoint( const LocalTimePoint& timePoint ) {
        return timePoint.isLocalTime() ?
                   GregorianCalendar::Date::fromLocalTime( timePoint.duration(), timePoint.offset() ) :
                   GregorianCalendar::Date::fromUtcTime( timePoint.duration(), timePoint.offset() );
    }

    static std::string failedAt( int codeLine ) {
        return "at: " + std::string{ __FILE__ } + ":" + std::to_string( codeLine );
    }

private:
    SunStateEvent::State m_sunState;
    SunStateEvent::Days m_days;
    SunStateEvent::Weekdays m_weekdays;
    std::unique_ptr< TestClock > m_clockPtr;
};

TEST_F( SunStateEventTest, toDuration ) {
    EXPECT_EQ( SunStateEvent::toDuration( "15" ), std::chrono::seconds( 15 ) );
    EXPECT_EQ( SunStateEvent::toDuration( "20:00" ), std::chrono::seconds( 20 * 60 ) );
    EXPECT_EQ( SunStateEvent::toDuration( "00:20:00" ), std::chrono::seconds( 20 * 60 ) );
    EXPECT_EQ( SunStateEvent::toDuration( "00:20:00" ), std::chrono::seconds( 20 * 60 ) );
    EXPECT_EQ( SunStateEvent::toDuration( "12:20:00" ), std::chrono::seconds( 12 * 60 * 60 + 20 * 60 ) );
    EXPECT_EQ( SunStateEvent::toDuration( "01:40:00" ), std::chrono::seconds( 1 * 60 * 60 + 40 * 60 ) );
    EXPECT_EQ( SunStateEvent::toDuration( "00:00:15" ), std::chrono::seconds( 15 ) );
    EXPECT_EQ( SunStateEvent::toDuration( "00:00:60" ), std::chrono::seconds( 60 ) );
    EXPECT_EQ( SunStateEvent::toDuration( "00:00:100" ), std::chrono::seconds( 100 ) );
    EXPECT_EQ( SunStateEvent::toDuration( "00:00:1000" ), std::chrono::seconds( 1000 ) );
    EXPECT_EQ( SunStateEvent::toDuration( "00:40:15" ), std::chrono::seconds( 40 * 60 + 15 ) );
    EXPECT_EQ( SunStateEvent::toDuration( "23:40:15" ), std::chrono::seconds( 23 * 60 * 60 + 40 * 60 + 15 ) );
    EXPECT_EQ( SunStateEvent::toDuration( "15:23:55" ), std::chrono::seconds( 15 * 60 * 60 + 23 * 60 + 55 ) );
    EXPECT_ANY_THROW( SunStateEvent::toDuration( "15:23:55:00" ) );
}

TEST_F( SunStateEventTest, setTimeOffset ) {
    SunStateEvent sunstateEvent( SunStateEvent::State::SUN_RISE,
                                 SunStateEvent::OffsetType::IN_TIME,
                                 []() {
                                     return HubPlatform::CoreDRemoteInterface::GeoPosition{ 1, 1 };
                                 },
                                 makeClockGetter( "", std::chrono::hours( 0 ) ) );
    EXPECT_NO_THROW( sunstateEvent.setTimeOffset( "00:20:00" ) );
    EXPECT_ANY_THROW( sunstateEvent.setTimeOffset( "15:23:55:00" ) );
    EXPECT_ANY_THROW( sunstateEvent.setTimeOffset( "string" ) );
    EXPECT_ANY_THROW( sunstateEvent.setTimeOffset( 10 ) );
}

TEST_F( SunStateEventTest, DailySunRiseTest ) {
    runTestCases( ReferenceRandomSunstatesList, SunStateEvent::State::SUN_RISE );
}

TEST_F( SunStateEventTest, WeeklySunRiseTest ) {
    testWeeklySunState( SunStateEvent::State::SUN_RISE, __LINE__ );
}

TEST_F( SunStateEventTest, MonthlySunRiseTest ) {
    testMonthlySunState( SunStateEvent::State::SUN_RISE, __LINE__ );
}

TEST_F( SunStateEventTest, DailySunSetTest ) {
    runTestCases( ReferenceRandomSunstatesList, SunStateEvent::State::SUN_SET );
}

TEST_F( SunStateEventTest, WeeklySunSetTest ) {
    testWeeklySunState( SunStateEvent::State::SUN_SET, __LINE__ );
}

TEST_F( SunStateEventTest, MonthlySunSetTest ) {
    testMonthlySunState( SunStateEvent::State::SUN_SET, __LINE__ );
}

TEST_F( SunStateEventTest, ConstructorExceptions ) {
    SunStateEvent::State state = SunStateEvent::State::SUN_RISE;
    std::shared_ptr< SunStateEvent > sunStateEvent;

    auto geoPositionGetter = []() { return HubPlatform::CoreDRemoteInterface::GeoPosition{ 49.99450, 36.21506 }; };
    auto clockGetter = makeClockGetter( "", std::chrono::hours( 3 ) );

    EXPECT_NO_THROW( sunStateEvent = std::make_shared< SunStateEvent >(
                         state, SunStateEvent::OffsetType::IN_TIME, geoPositionGetter, clockGetter ); )
        << "no exceptions of all parameters passed";

    EXPECT_THROW( sunStateEvent = std::make_shared< SunStateEvent >(
                      state, SunStateEvent::OffsetType::IN_TIME, nullptr, clockGetter ),
                  std::invalid_argument )
        << "exception on empty getGeoPositionCallback";

    EXPECT_THROW( sunStateEvent = std::make_shared< SunStateEvent >(
                      state, SunStateEvent::OffsetType::IN_TIME, geoPositionGetter, nullptr ),
                  std::invalid_argument )
        << "exceptions on empty clock getter";
}

class TimeZoneWrapper {
public:
    explicit TimeZoneWrapper( const std::string& timezone ) {
        auto originalTZ = getenv( "TZ" );
        if( originalTZ )
            m_prevTimezone = originalTZ;

        /** Need to use timezone with DST for test to catch a moment of DTS change. UTC doesn't have it. */
        setenv( "TZ", timezone.c_str(), 1 );
        LOGGER_DEBUG( timezone );
    }
    ~TimeZoneWrapper() {
        if( !m_prevTimezone.empty() ) {
            unsetenv( "TZ" );
        }
    }

private:
    std::string m_prevTimezone;
};

TEST_F( SunStateEventTest, DailyDSTSunRiseTest ) {
    const char* timezone = "Europe/Rome";
    {
        TimeZoneWrapper w( timezone );
        runTestCases( DSTSunStatesList, SunStateEvent::State::SUN_RISE );
    }
}

TEST_F( SunStateEventTest, DailyDSTSunSetTest ) {
    const char* timezone = "Europe/Rome";
    {
        TimeZoneWrapper w( timezone );
        runTestCases( DSTSunStatesList, SunStateEvent::State::SUN_SET );
    }
}

class DSTTests : public ::testing::Test {};

TEST_F( DSTTests, currentTimezoneHasDST ) {
    Clock clock;

    auto originalTZ = getenv( "TZ" );
    LOGGER_DEBUG( "original TZ: " + std::string( originalTZ ? originalTZ : "empty" ) );

    EXPECT_EQ( setenv( "TZ", "Europe/Rome", 1 ), 0 );
    LOGGER_DEBUG( "Europa/Rome" );
    auto now = clock.greenwichMeanTimePoint();
    auto timezone = clock.timeZoneOffset( now );

    EXPECT_TRUE( SceneEngine::DaylightSavingsTime::hasDSTChange( clock ) );

    EXPECT_EQ( setenv( "TZ", "Asia/Kathmandu", 1 ), 0 );
    LOGGER_DEBUG( "Asia/Kathmandu" );
    EXPECT_FALSE( DaylightSavingsTime::hasDSTChange( clock ) );

    EXPECT_EQ( setenv( "TZ", "Asia/Karachi", 1 ), 0 );
    LOGGER_DEBUG( "Asia/Karachi" );
    EXPECT_FALSE( DaylightSavingsTime::hasDSTChange( clock ) );

    // US, Arizona, Mountain Standard Time, no DST
    EXPECT_EQ( setenv( "TZ", "US/Arizona", 1 ), 0 );
    LOGGER_DEBUG( "Arizona" );
    EXPECT_FALSE( DaylightSavingsTime::hasDSTChange( clock ) );

    // US, New York, has DST
    EXPECT_EQ( setenv( "TZ", "America/New_York", 1 ), 0 );
    LOGGER_DEBUG( "New York" );
    EXPECT_TRUE( DaylightSavingsTime::hasDSTChange( clock ) );

    if( !originalTZ || !*originalTZ ) {
        unsetenv( "TZ" );
    }
}

TEST_F( DSTTests, currentTimezoneHasDSTTestClock ) {
    using namespace GregorianCalendar;
    Date date( "2023-03-26T00:00:00+0200" );
    LOGGER_DEBUG( "date serialized: " + date.toString( "%F %T %z" ) );
    auto localTimePoint = date.toTimePoint();  // + date.getTimeZoneOffset();

    using namespace std::chrono;

    auto originalTZ = getenv( "TZ" );
    LOGGER_DEBUG( "original TZ: " + std::string( originalTZ ? originalTZ : "" ) );

    {
        TestClock clock( date.toSeconds(), hours( 2 ) );
        EXPECT_EQ( setenv( "TZ", "Europe/Rome", 1 ), 0 );
        LOGGER_DEBUG( "Europa/Rome" );
        EXPECT_TRUE( SceneEngine::DaylightSavingsTime::hasDSTChange( clock ) );
    }
    {
        TestClock clock( date.toSeconds(), minutes( 5 * 60 + 45 ) );
        EXPECT_EQ( setenv( "TZ", "Asia/Kathmandu", 1 ), 0 );
        LOGGER_DEBUG( "Nepal" );
        EXPECT_FALSE( DaylightSavingsTime::hasDSTChange( clock ) );
    }

    {
        TestClock clock( date.toSeconds(), hours( 5 ) );
        EXPECT_EQ( setenv( "TZ", "Asia/Karachi", 1 ), 0 );
        LOGGER_DEBUG( "Pakistan" );
        EXPECT_FALSE( DaylightSavingsTime::hasDSTChange( clock ) );
    }
    {  // US, Arizona, Mountain Standard Time, no DST
        TestClock clock( date.toSeconds(), hours( -7 ) );
        EXPECT_EQ( setenv( "TZ", "US/Arizona", 1 ), 0 );
        LOGGER_DEBUG( "Arizona" );
        EXPECT_FALSE( DaylightSavingsTime::hasDSTChange( clock ) );
    }
    {  // US, New York, has DST
        TestClock clock( date.toSeconds(), hours( -4 ) );
        EXPECT_EQ( setenv( "TZ", "America/New_York", 1 ), 0 );
        LOGGER_DEBUG( "New York" );
        EXPECT_TRUE( DaylightSavingsTime::hasDSTChange( clock ) );
    }
    if( !originalTZ || !*originalTZ )
        unsetenv( "TZ" );
}

}  // namespace UT
}  // namespace SceneEngine
