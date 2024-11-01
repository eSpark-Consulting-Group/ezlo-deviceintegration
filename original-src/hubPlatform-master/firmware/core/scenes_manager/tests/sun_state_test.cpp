#include "scenes_manager/scenes_engine/sunstate_processor/sun_state.h"
#include "scenes_manager/tests/resources/sun_state_reference.h"
#include "base_utils/gregorian_calendar.h"
#include "log_utils/ezlo_log.h"

#include <gtest/gtest.h>
#include <tiny/tiny_consolecolors.h>

/*
 TESTPLAN

 1. isSunState with 1-hour length timezones, random data;
 2. isSunState with non-standard offsets (45 min, 30 min);

 */

void PrintTo( const SunState::Time& time, std::ostream* os ) {
    *os << std::setfill( '0' );
    *os << std::setw( 2 ) << time.hours << ":" << std::setw( 2 ) << time.minutes << ":";

    if( time.seconds != SunState::NaN )
        *os << std::setw( 2 ) << time.seconds;
    else
        *os << "NaN";
}

namespace SceneEngine {
namespace UT {

class SunStateTest : public testing::Test {};

TEST_F( SunStateTest, SunriseSunset ) /* 1 */ {
    SunState sunState;
    for( const TestData& data : ReferenceRandomSunstatesList ) {
        sunState.setPosition( data.geoposition.latitude, data.geoposition.longitude, data.timeZoneOffset );
        sunState.setCurrentDate( data.date.year, data.date.month, data.date.day );
        SunState::Time sunriseTime = sunState.getSunriseTime();
        EXPECT_EQ( sunriseTime.hours, data.sunrise.hours ) << data;
        EXPECT_EQ( sunriseTime.minutes, data.sunrise.minutes ) << data;
        SunState::Time sunsetTime = sunState.getSunsetTime();
        EXPECT_EQ( sunsetTime.hours, data.sunset.hours ) << data;
        EXPECT_EQ( sunsetTime.minutes, data.sunset.minutes ) << data;
    }
}

struct TestDataTZ : public TestData {
public:
    TestDataTZ( HubPlatform::CoreDRemoteInterface::GeoPosition gp,
                TestDate td,
                SunState::Time sunrise,
                SunState::Time sunset,
                SunState::TimeZoneOffset tzOffset,
                const char* location = "" ) {
        this->geoposition = gp;
        this->date = td;
        this->sunrise = sunrise;
        this->sunset = sunset;
        this->tzOffset = tzOffset;
        this->location = location;
    }

    friend std::ostream& operator<<( std::ostream& os, const TestDataTZ& p );

public:
    SunState::TimeZoneOffset tzOffset;
    std::string location;
};

std::string toString( const SunState::TimeZoneOffset& tzOffset ) {
    const auto tzMinutesOffset = tzOffset.getTimeZoneOffsetMinutes();
    if( tzMinutesOffset.count() == 0 )
        return "+0000";

    char buf[ 16 ];
    const char sign = tzOffset.getDirection() == SunState::TimeZoneOffset::Direction::AFTER ? '-' : '+';
    snprintf( buf, sizeof( buf ), "%c%.2u%.2uZ", sign, tzOffset.getHours(), tzOffset.getMinutes() );
    std::string result = buf;
    return result;
}

std::ostream& operator<<( std::ostream& os, const TestDataTZ& p ) {
    auto location = p.location;
    if( location.empty() )
        location = "not set";
    return os << "GeoPosition( " << p.geoposition.latitude << ", " << p.geoposition.longitude << " ), "
              << "TimeZoneOffset( " << toString( p.tzOffset ) << " ), "
              << "Date( " << p.date.year << "-" << p.date.month << "-" << p.date.day << " ), "
              << "SunriseTime( " << p.sunrise.hours << ":" << p.sunrise.minutes << ":" << p.sunrise.seconds << " ), "
              << "SunsetTime( " << p.sunset.hours << ":" << p.sunset.minutes << ":" << p.sunset.seconds << " ), "
              << "Location( " << location << " )";
}

TEST_F( SunStateTest, sunbStateForNonStandardOffsets ) /* 2 */ {
    // (https://en.wikipedia.org/wiki/List_of_UTC_offsets)

    /**
     * TimeZoneOffset:
     *      5, 30, true => +0530 (West of Greenwich);
     *      5, 45, false => -0545 (East of Greenwich)
     * */

    const auto AFTER = SunState::TimeZoneOffset::Direction::AFTER;
    const auto BEFORE = SunState::TimeZoneOffset::Direction::BEFORE;
    const std::vector< TestDataTZ > FractionalTimeZonesSunStateList = {
        // GeoPosition           Date             SunriseTime    SunsetTime          TimeZoneOffset
        { { 25.7702, -80.1672 },
          { 2019, 1, 15 },
          { 7, 8, 56 },
          { 17, 51, 21 },
          { 5, 0, AFTER },
          "Dodge Island, Miami, FL, USA" },
        { { 27.69237, 85.31644 }, { 2023, 3, 19 }, { 6, 9, 13 }, { 18, 14, 27 }, { 5, 45, BEFORE }, "Nepal/Kathmandu" },
        { { 27.69237, 85.31644 }, { 2023, 6, 19 }, { 5, 7, 52 }, { 19, 2, 10 }, { 5, 45, BEFORE }, "Nepal/Kathmandu" },
        { { 27.69237, 85.31644 },
          { 2023, 9, 19 },
          { 5, 50, 15 },
          { 18, 04, 43 },
          { 5, 45, BEFORE },
          "Nepal/Kathmandu" },
        { { 27.69237, 85.31644 },
          { 2023, 12, 19 },
          { 6, 48, 28 },
          { 17, 12, 37 },
          { 5, 45, BEFORE },
          "Nepal/Kathmandu" },
    };

    for( const TestDataTZ& data : FractionalTimeZonesSunStateList ) {
        SunState sunState;
        sunState.setPosition( data.geoposition.latitude, data.geoposition.longitude, data.tzOffset );
        sunState.setCurrentDate( data.date.year, data.date.month, data.date.day );
        SunState::Time sunriseTime = sunState.getSunriseTime();

        EXPECT_LE( abs( sunriseTime.toSeconds() - data.sunrise.toSeconds() ), 60 )
            << "Expected: " << data.sunrise << ", actual: " << sunriseTime << ", location: " << data.location;
        SunState::Time sunsetTime = sunState.getSunsetTime();
        EXPECT_LE( abs( sunsetTime.toSeconds() - data.sunset.toSeconds() ), 60 )
            << "Expected: " << data.sunrise << ", actual: " << sunriseTime << ", location: " << data.location;
    }
}

}  // namespace UT
}  // namespace SceneEngine
