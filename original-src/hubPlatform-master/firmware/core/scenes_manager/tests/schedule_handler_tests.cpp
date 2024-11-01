#include "scenes_manager/scenes_engine/date_scheduler/schedule_handler.h"
#include "scenes_manager/scenes_engine/events/date_event.h"
#include "common_utils/cpp14.h"
#include "log_utils/ezlo_log.h"
#include "scenes_manager/scenes_engine/helpers/clock_interface.h"

#include <gtest/gtest.h>

/*

TEST PLAN

DONE    1. Date scheduler / Schedule point / isLess
DONE        1.1 isLess Utc < Self
DONE        1.2 isLess local < local
DONE        1.3 isLess Local < Utc
DONE        1.4 isLess Utc < Local
DONE        1.5 isLess local < local, diff offset

 */

namespace SceneEngine {
namespace UT {

class SchedulePoint_isLess : public testing::Test {};

TEST_F( SchedulePoint_isLess, isLess_Utc_Self ) /* 1.1 */ {
    using namespace std::chrono;

    {  // isLess false for equals
        GregorianCalendar::Date date1( "2023-10-25T10:00:00+0000" );
        LocalTimePoint time1( date1.toClockDuration(), seconds( 0 ) );

        ScheduleHandler::SchedulePoint point1 = { "scene1", "name1", "isDate", time1 };

        ScheduleHandler::SchedulePoint point2 = { "scene2", "name2", "isDate", time1 };

        EXPECT_FALSE( point1.isLess( point2 ) ) << "isLess is false for equal time points";
        EXPECT_FALSE( point2.isLess( point1 ) ) << "isLess is false for equal time points";
    }
}

TEST_F( SchedulePoint_isLess, isLess_Local_Local ) /* 1.2 */ {
    using namespace std::chrono;

    {  // isLess true, local vs local
        GregorianCalendar::Date date1( "2023-10-25T10:00:00+0200" );
        LocalTimePoint time1(
            GregorianCalendar::Date::toLocalTime( date1.toClockDuration(), date1.getTimeZoneOffset() ),
            date1.getTimeZoneOffset(),
            true );

        ScheduleHandler::SchedulePoint point1 = { "scene1", "name1", "isDate", time1 };

        GregorianCalendar::Date date2( "2023-10-25T10:10:00+0200" );
        LocalTimePoint time2(
            GregorianCalendar::Date::toLocalTime( date2.toClockDuration(), date2.getTimeZoneOffset() ),
            date2.getTimeZoneOffset(),
            true );
        ScheduleHandler::SchedulePoint point2 = { "scene2", "name2", "isDate", time2 };

        EXPECT_TRUE( point1.isLess( point2 ) )
            << "isLess is true for two local time points: " << date1.toString() << " less " << date2.toString();
        EXPECT_FALSE( point2.isLess( point1 ) )
            << "isLess is false for two local time points: " << date2.toString() << " less " << date1.toString();
    }
}

TEST_F( SchedulePoint_isLess, isLess_Local_Utc ) /* 1.3 */ {
    using namespace std::chrono;

    {  // isLess true, local vs utc
        GregorianCalendar::Date date1( "2023-10-25T10:00:00+0200" );
        LocalTimePoint time1(
            GregorianCalendar::Date::toLocalTime( date1.toClockDuration(), date1.getTimeZoneOffset() ),
            date1.getTimeZoneOffset(),
            true );

        ScheduleHandler::SchedulePoint point1 = { "scene1", "name1", "isDate", time1 };

        GregorianCalendar::Date date2( "2023-10-25T12:10:00+0000" );
        LocalTimePoint time2( date2.toClockDuration(), seconds( 0 ), false );
        ScheduleHandler::SchedulePoint point2 = { "scene2", "name2", "isDate", time2 };

        EXPECT_TRUE( point1.isLess( point2 ) )
            << "isLess is true for local vs utc time points: " << date1.toString() << " less " << date2.toString();
        EXPECT_FALSE( point2.isLess( point1 ) )
            << "isLess is false for uts va local time points: " << date2.toString() << " less " << date1.toString();
    }
}

TEST_F( SchedulePoint_isLess, isLess_Utc_Local ) /* 1.4 */ {
    using namespace std::chrono;

    {  // isLess true, utc vs local
        GregorianCalendar::Date date1( "2023-10-25T08:00:00+0000" );
        LocalTimePoint time1( date1.toClockDuration(), seconds( 0 ), false );

        ScheduleHandler::SchedulePoint point1 = { "scene1", "name1", "isDate", time1 };

        GregorianCalendar::Date date2( "2023-10-25T10:10:00+0200" );
        LocalTimePoint time2(
            GregorianCalendar::Date::toLocalTime( date2.toClockDuration(), date2.getTimeZoneOffset() ),
            date2.getTimeZoneOffset(),
            true );
        ScheduleHandler::SchedulePoint point2 = { "scene2", "name2", "isDate", time2 };

        EXPECT_TRUE( point1.isLess( point2 ) )
            << "isLess is true for utc vs local time points: " << date1.toString() << " less " << date2.toString();
        EXPECT_FALSE( point2.isLess( point1 ) )
            << "isLess is true for utc vs local time points: " << date1.toString() << " less " << date2.toString();
    }
}

TEST_F( SchedulePoint_isLess, isLess_Local_Local_DiffOffset ) /* 1.5 */ {
    using namespace std::chrono;

    {  // isLess true, local vs local
        GregorianCalendar::Date date1( "2023-10-25T10:00:00+0200" );
        LocalTimePoint time1(
            GregorianCalendar::Date::toLocalTime( date1.toClockDuration(), date1.getTimeZoneOffset() ),
            date1.getTimeZoneOffset(),
            true );

        ScheduleHandler::SchedulePoint point1 = { "scene1", "name1", "isDate", time1 };

        GregorianCalendar::Date date2( "2023-10-25T09:10:00+0100" );
        LocalTimePoint time2(
            GregorianCalendar::Date::toLocalTime( date2.toClockDuration(), date2.getTimeZoneOffset() ),
            date2.getTimeZoneOffset(),
            true );
        ScheduleHandler::SchedulePoint point2 = { "scene2", "name2", "isDate", time2 };

        EXPECT_TRUE( point1.isLess( point2 ) )
            << "isLess is true for two local time points: " << date1.toString() << " less " << date2.toString();
        EXPECT_FALSE( point2.isLess( point1 ) )
            << "isLess is false for two local time points: " << date2.toString() << " less " << date1.toString();
    }
}

}  // namespace UT
}  // namespace SceneEngine