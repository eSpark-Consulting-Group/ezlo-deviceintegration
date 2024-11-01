#include "scenes_manager/scenes_engine/events/date_range_event.h"

#include <gtest/gtest.h>

namespace SceneEngine {
namespace UT {

class DataRangeEventTests : public testing::Test {
public:
    bool isInYearRange( const GregorianCalendar::Date& date ) const {
        return dateRangeEvent.isInYearRange( date );
    }

    bool isInMonthRange( const GregorianCalendar::Date& date ) const {
        return dateRangeEvent.isInMonthRange( date );
    }

    bool isInDayRange( const GregorianCalendar::Date& date ) const {
        return dateRangeEvent.isInDayRange( date );
    }

    bool isInTimeRange( const GregorianCalendar::Date& date ) const {
        return dateRangeEvent.isInTimeRange( date );
    }

    bool crossedRangeEdge( const GregorianCalendar::TimePoint& localNow, std::chrono::seconds tzOffset ) {
        return dateRangeEvent.update( localNow, tzOffset, true );
    }

    DateRangeEvent::ScheduleType getScheduleType() const {
        return dateRangeEvent.getScheduleType();
    }

    DateRangeEvent dateRangeEvent;
};

TEST_F( DataRangeEventTests, getScheduleType ) {
    ASSERT_TRUE( dateRangeEvent.setMinuteRange( 15, 55 ) );
    ASSERT_TRUE( dateRangeEvent.setHourRange( 9, 11 ) );

    EXPECT_EQ( getScheduleType(), DateRangeEvent::ScheduleType::Daily );

    ASSERT_TRUE( dateRangeEvent.setMonthAndDayRanges( { 1, 12 }, { 5, 5 } ) );

    EXPECT_EQ( getScheduleType(), DateRangeEvent::ScheduleType::Monthly );

    ASSERT_TRUE( dateRangeEvent.setMonthAndDayRanges( { 5, 7 }, { 5, 23 } ) );

    EXPECT_EQ( getScheduleType(), DateRangeEvent::ScheduleType::Yearly );

    ASSERT_TRUE( dateRangeEvent.setYearRange( 2021, 2022 ) );

    EXPECT_EQ( getScheduleType(), DateRangeEvent::ScheduleType::Years );
}

TEST_F( DataRangeEventTests, isInTimeRange ) {
    using namespace GregorianCalendar;
    ASSERT_TRUE( dateRangeEvent.setMinuteRange( 15, 55 ) );
    ASSERT_TRUE( dateRangeEvent.setHourRange( 9, 11 ) );

    EXPECT_TRUE( isInTimeRange( Date( "2021-01-01T09:15:00+0000" ) ) );
    EXPECT_TRUE( isInTimeRange( Date( "2021-01-01T10:00:00+0000" ) ) );
    EXPECT_TRUE( isInTimeRange( Date( "2021-01-01T11:54:00+0000" ) ) );

    EXPECT_TRUE( isInTimeRange( Date( "2021-01-02T09:15:00+0000" ) ) );
    EXPECT_TRUE( isInTimeRange( Date( "2021-01-02T10:00:00+0000" ) ) );
    EXPECT_TRUE( isInTimeRange( Date( "2021-01-02T11:54:00+0000" ) ) );

    EXPECT_TRUE( isInTimeRange( Date( "2021-01-03T09:15:00+0000" ) ) );
    EXPECT_TRUE( isInTimeRange( Date( "2021-01-03T10:00:00+0000" ) ) );
    EXPECT_TRUE( isInTimeRange( Date( "2021-01-03T11:54:00+0000" ) ) );

    EXPECT_TRUE( isInTimeRange( Date( "2021-05-02T09:15:00+0000" ) ) );
    EXPECT_TRUE( isInTimeRange( Date( "2021-05-02T10:00:00+0000" ) ) );
    EXPECT_TRUE( isInTimeRange( Date( "2021-05-02T11:54:00+0000" ) ) );

    EXPECT_FALSE( isInTimeRange( Date( "2021-01-01T09:14:00+0000" ) ) );
    EXPECT_FALSE( isInTimeRange( Date( "2021-01-01T11:55:00+0000" ) ) );

    EXPECT_FALSE( isInTimeRange( Date( "2021-04-11T09:14:00+0000" ) ) );
    EXPECT_FALSE( isInTimeRange( Date( "2021-04-11T11:55:00+0000" ) ) );

    ASSERT_TRUE( dateRangeEvent.setMinuteRange( 55, 15 ) );
    ASSERT_TRUE( dateRangeEvent.setHourRange( 11, 9 ) );

    EXPECT_TRUE( isInTimeRange( Date( "2021-01-01T00:14:00+0000" ) ) );
    EXPECT_TRUE( isInTimeRange( Date( "2021-01-01T05:14:00+0000" ) ) );
    EXPECT_TRUE( isInTimeRange( Date( "2021-01-01T09:14:00+0000" ) ) );
    EXPECT_TRUE( isInTimeRange( Date( "2021-01-01T11:55:00+0000" ) ) );
    EXPECT_TRUE( isInTimeRange( Date( "2021-01-01T16:55:00+0000" ) ) );
    EXPECT_TRUE( isInTimeRange( Date( "2021-01-01T23:55:00+0000" ) ) );
    EXPECT_TRUE( isInTimeRange( Date( "2021-01-02T09:14:00+0000" ) ) );

    EXPECT_FALSE( isInTimeRange( Date( "2021-01-01T09:15:00+0000" ) ) );
    EXPECT_FALSE( isInTimeRange( Date( "2021-01-01T10:23:00+0000" ) ) );
    EXPECT_FALSE( isInTimeRange( Date( "2021-01-01T11:54:00+0000" ) ) );

    EXPECT_FALSE( isInTimeRange( Date( "2021-02-01T09:15:00+0000" ) ) );
    EXPECT_FALSE( isInTimeRange( Date( "2021-02-01T10:23:00+0000" ) ) );
    EXPECT_FALSE( isInTimeRange( Date( "2021-02-01T11:54:00+0000" ) ) );
}

TEST_F( DataRangeEventTests, isInDayRange ) {
    using namespace GregorianCalendar;
    ASSERT_TRUE( dateRangeEvent.setMinuteRange( 15, 55 ) );
    ASSERT_TRUE( dateRangeEvent.setHourRange( 9, 11 ) );
    ASSERT_TRUE( dateRangeEvent.setMonthAndDayRanges( { 1, 12 }, { 5, 5 } ) );

    EXPECT_TRUE( isInDayRange( Date( "2021-01-05T10:23:00+0000" ) ) );
    EXPECT_TRUE( isInDayRange( Date( "2021-02-05T10:23:00+0000" ) ) );
    EXPECT_TRUE( isInDayRange( Date( "2021-01-05T09:15:00+0000" ) ) );
    EXPECT_TRUE( isInDayRange( Date( "2021-12-05T11:50:00+0000" ) ) );
    EXPECT_TRUE( isInDayRange( Date( "2021-12-05T11:54:00+0000" ) ) );

    EXPECT_FALSE( isInDayRange( Date( "2021-01-05T09:14:00+0000" ) ) );
    EXPECT_FALSE( isInDayRange( Date( "2021-01-05T11:56:00+0000" ) ) );
    EXPECT_FALSE( isInDayRange( Date( "2021-04-05T09:14:00+0000" ) ) );
    EXPECT_FALSE( isInDayRange( Date( "2021-04-05T11:56:00+0000" ) ) );
    EXPECT_FALSE( isInDayRange( Date( "2021-01-06T10:23:00+0000" ) ) );
    EXPECT_FALSE( isInDayRange( Date( "2021-01-04T10:23:00+0000" ) ) );
    EXPECT_FALSE( isInDayRange( Date( "2021-02-01T10:23:00+0000" ) ) );
    EXPECT_FALSE( isInDayRange( Date( "2021-10-15T10:23:00+0000" ) ) );
    EXPECT_FALSE( isInDayRange( Date( "2022-01-01T10:23:00+0000" ) ) );
    EXPECT_FALSE( isInDayRange( Date( "2021-12-05T11:55:00+0000" ) ) );

    ASSERT_TRUE( dateRangeEvent.setMonthAndDayRanges( { 1, 12 }, { 5, 23 } ) );

    EXPECT_TRUE( isInDayRange( Date( "2021-01-05T09:15:00+0000" ) ) );
    EXPECT_TRUE( isInDayRange( Date( "2021-01-05T11:54:00+0000" ) ) );
    EXPECT_TRUE( isInDayRange( Date( "2021-01-05T16:00:00+0000" ) ) );
    EXPECT_TRUE( isInDayRange( Date( "2021-01-22T16:00:00+0000" ) ) );
    EXPECT_TRUE( isInDayRange( Date( "2021-01-23T11:54:00+0000" ) ) );

    EXPECT_TRUE( isInDayRange( Date( "2021-02-05T09:15:00+0000" ) ) );
    EXPECT_TRUE( isInDayRange( Date( "2021-02-05T11:55:00+0000" ) ) );
    EXPECT_TRUE( isInDayRange( Date( "2021-02-05T16:00:00+0000" ) ) );
    EXPECT_TRUE( isInDayRange( Date( "2021-02-22T16:00:00+0000" ) ) );
    EXPECT_TRUE( isInDayRange( Date( "2021-02-23T11:54:00+0000" ) ) );

    EXPECT_TRUE( isInDayRange( Date( "2021-12-05T09:15:00+0000" ) ) );
    EXPECT_TRUE( isInDayRange( Date( "2021-12-05T11:54:00+0000" ) ) );
    EXPECT_TRUE( isInDayRange( Date( "2021-12-05T16:00:00+0000" ) ) );
    EXPECT_TRUE( isInDayRange( Date( "2021-12-22T16:00:00+0000" ) ) );
    EXPECT_TRUE( isInDayRange( Date( "2021-12-23T11:54:00+0000" ) ) );

    EXPECT_TRUE( isInDayRange( Date( "2022-12-05T09:15:00+0000" ) ) );
    EXPECT_TRUE( isInDayRange( Date( "2022-12-05T11:54:00+0000" ) ) );
    EXPECT_TRUE( isInDayRange( Date( "2022-12-05T16:00:00+0000" ) ) );
    EXPECT_TRUE( isInDayRange( Date( "2022-12-22T16:00:00+0000" ) ) );
    EXPECT_TRUE( isInDayRange( Date( "2022-12-23T11:54:00+0000" ) ) );

    EXPECT_FALSE( isInDayRange( Date( "2021-01-01T09:20:00+0000" ) ) );
    EXPECT_FALSE( isInDayRange( Date( "2021-01-05T09:14:00+0000" ) ) );
    EXPECT_FALSE( isInDayRange( Date( "2021-01-23T11:55:00+0000" ) ) );
    EXPECT_FALSE( isInDayRange( Date( "2021-01-25T11:00:00+0000" ) ) );

    EXPECT_FALSE( isInDayRange( Date( "2021-05-01T09:20:00+0000" ) ) );
    EXPECT_FALSE( isInDayRange( Date( "2021-05-05T09:14:00+0000" ) ) );
    EXPECT_FALSE( isInDayRange( Date( "2021-05-23T11:55:00+0000" ) ) );
    EXPECT_FALSE( isInDayRange( Date( "2021-05-25T11:00:00+0000" ) ) );

    ASSERT_TRUE( dateRangeEvent.setMonthAndDayRanges( { 1, 12 }, { 23, 5 } ) );

    EXPECT_TRUE( isInDayRange( Date( "2021-01-01T09:00:00+0000" ) ) );
    EXPECT_TRUE( isInDayRange( Date( "2021-01-05T09:15:00+0000" ) ) );
    EXPECT_TRUE( isInDayRange( Date( "2021-01-23T11:54:00+0000" ) ) );
    EXPECT_TRUE( isInDayRange( Date( "2021-01-25T15:54:00+0000" ) ) );
    EXPECT_TRUE( isInDayRange( Date( "2021-02-01T05:54:00+0000" ) ) );
    EXPECT_TRUE( isInDayRange( Date( "2021-02-01T09:15:00+0000" ) ) );

    EXPECT_FALSE( isInDayRange( Date( "2021-01-05T11:55:00+0000" ) ) );
    EXPECT_FALSE( isInDayRange( Date( "2021-01-10T16:15:00+0000" ) ) );
    EXPECT_FALSE( isInDayRange( Date( "2021-01-23T09:14:00+0000" ) ) );

    EXPECT_FALSE( isInDayRange( Date( "2021-02-05T11:55:00+0000" ) ) );
    EXPECT_FALSE( isInDayRange( Date( "2021-02-10T16:15:00+0000" ) ) );
    EXPECT_FALSE( isInDayRange( Date( "2021-02-23T09:14:00+0000" ) ) );
}

TEST_F( DataRangeEventTests, isInMonthRange ) {
    using namespace GregorianCalendar;
    ASSERT_TRUE( dateRangeEvent.setMinuteRange( 15, 55 ) );
    ASSERT_TRUE( dateRangeEvent.setHourRange( 9, 11 ) );
    ASSERT_TRUE( dateRangeEvent.setMonthAndDayRanges( { 1, 12 }, { 5, 23 } ) );

    EXPECT_TRUE( isInMonthRange( Date( "2021-01-05T09:15:00+0000" ) ) );
    EXPECT_TRUE( isInMonthRange( Date( "2021-01-05T11:54:00+0000" ) ) );
    EXPECT_TRUE( isInMonthRange( Date( "2021-01-05T16:00:00+0000" ) ) );
    EXPECT_TRUE( isInMonthRange( Date( "2021-01-22T16:00:00+0000" ) ) );
    EXPECT_TRUE( isInMonthRange( Date( "2021-01-23T11:54:00+0000" ) ) );
    EXPECT_TRUE( isInMonthRange( Date( "2021-01-24T16:00:00+0000" ) ) );
    EXPECT_TRUE( isInMonthRange( Date( "2021-02-24T16:00:00+0000" ) ) );
    EXPECT_TRUE( isInMonthRange( Date( "2021-05-24T16:00:00+0000" ) ) );
    EXPECT_TRUE( isInMonthRange( Date( "2021-12-23T11:54:00+0000" ) ) );

    EXPECT_FALSE( isInMonthRange( Date( "2022-01-01T09:20:00+0000" ) ) );
    EXPECT_FALSE( isInMonthRange( Date( "2022-01-05T09:14:00+0000" ) ) );
    EXPECT_FALSE( isInMonthRange( Date( "2022-12-23T11:55:00+0000" ) ) );
    EXPECT_FALSE( isInMonthRange( Date( "2022-12-25T11:00:00+0000" ) ) );

    ASSERT_TRUE( dateRangeEvent.setMonthAndDayRanges( { 5, 7 }, { 5, 23 } ) );

    EXPECT_TRUE( isInMonthRange( Date( "2021-05-05T09:15:00+0000" ) ) );
    EXPECT_TRUE( isInMonthRange( Date( "2021-05-05T11:56:00+0000" ) ) );
    EXPECT_TRUE( isInMonthRange( Date( "2021-05-05T16:00:00+0000" ) ) );
    EXPECT_TRUE( isInMonthRange( Date( "2021-06-05T16:00:00+0000" ) ) );
    EXPECT_TRUE( isInMonthRange( Date( "2021-07-22T16:00:00+0000" ) ) );
    EXPECT_TRUE( isInMonthRange( Date( "2021-07-23T11:54:00+0000" ) ) );

    EXPECT_FALSE( isInMonthRange( Date( "2021-01-05T09:15:00+0000" ) ) );
    EXPECT_FALSE( isInMonthRange( Date( "2021-05-05T09:14:00+0000" ) ) );
    EXPECT_FALSE( isInMonthRange( Date( "2021-07-23T11:55:00+0000" ) ) );
    EXPECT_FALSE( isInMonthRange( Date( "2021-08-05T11:00:00+0000" ) ) );

    ASSERT_TRUE( dateRangeEvent.setMonthAndDayRanges( { 7, 5 }, { 5, 23 } ) );

    EXPECT_TRUE( isInMonthRange( Date( "2021-01-05T09:15:00+0000" ) ) );
    EXPECT_TRUE( isInMonthRange( Date( "2021-05-23T11:54:00+0000" ) ) );
    EXPECT_TRUE( isInMonthRange( Date( "2021-07-05T09:15:00+0000" ) ) );
    EXPECT_TRUE( isInMonthRange( Date( "2021-10-05T09:15:00+0000" ) ) );
    EXPECT_TRUE( isInMonthRange( Date( "2021-12-05T09:15:00+0000" ) ) );
    EXPECT_TRUE( isInMonthRange( Date( "2022-03-05T09:15:00+0000" ) ) );
    EXPECT_TRUE( isInMonthRange( Date( "2022-05-05T23:54:00+0000" ) ) );

    EXPECT_FALSE( isInMonthRange( Date( "2021-05-23T11:55:00+0000" ) ) );
    EXPECT_FALSE( isInMonthRange( Date( "2021-06-23T11:55:00+0000" ) ) );
    EXPECT_FALSE( isInMonthRange( Date( "2021-07-05T09:14:00+0000" ) ) );

    EXPECT_FALSE( isInMonthRange( Date( "2022-05-23T11:55:00+0000" ) ) );
    EXPECT_FALSE( isInMonthRange( Date( "2022-06-23T11:55:00+0000" ) ) );
    EXPECT_FALSE( isInMonthRange( Date( "2022-07-05T09:14:00+0000" ) ) );
}

TEST_F( DataRangeEventTests, isInYearRange ) {
    using namespace GregorianCalendar;
    ASSERT_TRUE( dateRangeEvent.setMinuteRange( 15, 55 ) );
    ASSERT_TRUE( dateRangeEvent.setHourRange( 9, 11 ) );
    ASSERT_TRUE( dateRangeEvent.setMonthAndDayRanges( { 5, 7 }, { 5, 23 } ) );
    ASSERT_TRUE( dateRangeEvent.setYearRange( 2021, 2022 ) );

    EXPECT_TRUE( isInYearRange( Date( "2021-05-05T09:15:00+0000" ) ) );
    EXPECT_TRUE( isInYearRange( Date( "2021-08-01T16:00:00+0000" ) ) );
    EXPECT_TRUE( isInYearRange( Date( "2021-10-27T16:00:00+0000" ) ) );
    EXPECT_TRUE( isInYearRange( Date( "2021-12-27T16:00:00+0000" ) ) );
    EXPECT_TRUE( isInYearRange( Date( "2022-01-27T16:00:00+0000" ) ) );
    EXPECT_TRUE( isInYearRange( Date( "2022-03-27T16:00:00+0000" ) ) );
    EXPECT_TRUE( isInYearRange( Date( "2022-06-27T16:00:00+0000" ) ) );
    EXPECT_TRUE( isInYearRange( Date( "2022-07-23T11:54:00+0000" ) ) );

    EXPECT_FALSE( isInYearRange( Date( "2021-01-05T09:15:00+0000" ) ) );
    EXPECT_FALSE( isInYearRange( Date( "2021-05-05T09:14:00+0000" ) ) );
    EXPECT_FALSE( isInYearRange( Date( "2022-07-23T11:55:00+0000" ) ) );
    EXPECT_FALSE( isInYearRange( Date( "2022-08-10T11:00:00+0000" ) ) );

    ASSERT_TRUE( dateRangeEvent.setYearRange( 2022, 2021 ) );

    EXPECT_TRUE( isInYearRange( Date( "2021-01-05T09:15:00+0000" ) ) );
    EXPECT_TRUE( isInYearRange( Date( "2021-07-23T11:54:00+0000" ) ) );
    EXPECT_TRUE( isInYearRange( Date( "2022-05-05T09:15:00+0000" ) ) );
    EXPECT_TRUE( isInYearRange( Date( "2022-12-23T11:55:00+0000" ) ) );
    EXPECT_TRUE( isInYearRange( Date( "2050-12-23T11:55:00+0000" ) ) );

    EXPECT_FALSE( isInYearRange( Date( "2021-07-23T11:55:00+0000" ) ) );
    EXPECT_FALSE( isInYearRange( Date( "2021-08-05T09:15:00+0000" ) ) );
    EXPECT_FALSE( isInYearRange( Date( "2022-03-05T09:15:00+0000" ) ) );
    EXPECT_FALSE( isInYearRange( Date( "2022-05-05T09:14:00+0000" ) ) );
}

TEST_F( DataRangeEventTests, crossedRangeEdge ) {
    using namespace GregorianCalendar;
    ASSERT_TRUE( dateRangeEvent.setMinuteRange( 15, 55 ) );
    ASSERT_TRUE( dateRangeEvent.setHourRange( 9, 11 ) );

    Date date;

    date = Date( "2021-01-01T09:15:00+0000" );
    EXPECT_TRUE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );
    date = Date( "2021-01-01T10:00:00+0000" );
    EXPECT_FALSE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );
    date = Date( "2021-01-01T11:55:00+0000" );
    EXPECT_TRUE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );

    date = Date( "2021-01-02T09:15:00+0000" );
    EXPECT_TRUE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );
    date = Date( "2021-01-02T10:00:00+0000" );
    EXPECT_FALSE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );
    date = Date( "2021-01-02T11:55:00+0000" );
    EXPECT_TRUE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );
    date = Date( "2021-01-02T11:55:00+0000" );
    EXPECT_FALSE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );

    ASSERT_TRUE( dateRangeEvent.setMonthAndDayRanges( { 1, 12 }, { 5, 23 } ) );

    date = Date( "2021-01-05T09:15:00+0000" );
    EXPECT_TRUE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );
    date = Date( "2021-01-05T11:54:00+0000" );
    EXPECT_FALSE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );
    date = Date( "2021-01-05T16:00:00+0000" );
    EXPECT_FALSE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );
    date = Date( "2021-01-22T16:00:00+0000" );
    EXPECT_FALSE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );
    date = Date( "2021-01-23T11:55:00+0000" );
    EXPECT_TRUE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );
    date = Date( "2021-01-23T11:55:00+0000" );
    EXPECT_FALSE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );

    date = Date( "2021-02-05T09:15:00+0000" );
    EXPECT_TRUE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );
    date = Date( "2021-02-05T11:55:00+0000" );
    EXPECT_FALSE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );
    date = Date( "2021-02-05T16:00:00+0000" );
    EXPECT_FALSE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );
    date = Date( "2021-02-22T16:00:00+0000" );
    EXPECT_FALSE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );
    date = Date( "2021-02-23T11:55:00+0000" );
    EXPECT_TRUE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );
    date = Date( "2021-02-23T11:55:00+0000" );
    EXPECT_FALSE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );

    ASSERT_TRUE( dateRangeEvent.setMonthAndDayRanges( { 5, 7 }, { 5, 23 } ) );

    date = Date( "2021-05-05T09:15:00+0000" );
    EXPECT_TRUE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );
    date = Date( "2021-05-05T11:56:00+0000" );
    EXPECT_FALSE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );
    date = Date( "2021-05-05T16:00:00+0000" );
    EXPECT_FALSE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );
    date = Date( "2021-06-05T16:00:00+0000" );
    EXPECT_FALSE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );
    date = Date( "2021-07-22T16:00:00+0000" );
    EXPECT_FALSE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );
    date = Date( "2021-07-23T11:55:00+0000" );
    EXPECT_TRUE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );
    date = Date( "2021-07-23T11:55:00+0000" );
    EXPECT_FALSE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );

    date = Date( "2022-05-05T09:15:00+0000" );
    EXPECT_TRUE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );
    date = Date( "2022-05-05T11:56:00+0000" );
    EXPECT_FALSE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );
    date = Date( "2022-05-05T16:00:00+0000" );
    EXPECT_FALSE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );
    date = Date( "2022-06-05T16:00:00+0000" );
    EXPECT_FALSE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );
    date = Date( "2022-07-22T16:00:00+0000" );
    EXPECT_FALSE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );
    date = Date( "2022-07-23T11:55:00+0000" );
    EXPECT_TRUE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );
    date = Date( "2022-07-23T11:55:00+0000" );
    EXPECT_FALSE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );

    ASSERT_TRUE( dateRangeEvent.setYearRange( 2021, 2022 ) );

    date = Date( "2021-05-05T09:15:00+0000" );
    EXPECT_TRUE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );
    date = Date( "2021-08-01T16:00:00+0000" );
    EXPECT_FALSE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );
    date = Date( "2021-10-27T16:00:00+0000" );
    EXPECT_FALSE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );
    date = Date( "2021-12-27T16:00:00+0000" );
    EXPECT_FALSE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );
    date = Date( "2022-01-27T16:00:00+0000" );
    EXPECT_FALSE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );
    date = Date( "2022-03-27T16:00:00+0000" );
    EXPECT_FALSE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );
    date = Date( "2022-06-27T16:00:00+0000" );
    EXPECT_FALSE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );
    date = Date( "2022-07-23T11:55:00+0000" );
    EXPECT_TRUE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );
    date = Date( "2022-07-23T11:55:00+0000" );
    EXPECT_FALSE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );

    date = Date( "2021-05-05T09:15:00+0000" );
    EXPECT_TRUE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );
    date = Date( "2021-05-05T09:15:00+0000" );
    EXPECT_FALSE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );
    date = Date( "2022-07-23T11:55:00+0000" );
    EXPECT_TRUE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );
    date = Date( "2022-07-23T11:55:00+0000" );
    EXPECT_FALSE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );

    ASSERT_TRUE( dateRangeEvent.setMinuteRange( 20, 10 ) );
    ASSERT_TRUE( dateRangeEvent.setHourRange( 15, 10 ) );
    ASSERT_TRUE( dateRangeEvent.setMonthAndDayRanges( { 3, 9 }, { 5, 2 } ) );
    ASSERT_TRUE( dateRangeEvent.setYearRange( 2021, 2022 ) );

    date = Date( "2021-03-05T15:20:00+0000" );
    EXPECT_TRUE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );
    date = Date( "2021-03-06T15:20:00+0000" );
    EXPECT_FALSE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );
    date = Date( "2022-09-02T10:10:00+0000" );
    EXPECT_TRUE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );
    date = Date( "2022-09-02T10:10:00+0000" );
    EXPECT_FALSE( crossedRangeEdge( date.toTimePoint(), date.getTimeZoneOffset() ) );
}

TEST_F( DataRangeEventTests, daily ) {
    using namespace GregorianCalendar;
    DateRangeEvent dateRangeEvent;
    ASSERT_TRUE( dateRangeEvent.setMinuteRange( 23, 24 ) );
    ASSERT_TRUE( dateRangeEvent.setHourRange( 10, 15 ) );

    const auto tzOffset = std::chrono::seconds( 0 );

    auto nextDate = dateRangeEvent.getNextDate( Date( "2021-03-30T09:12:44+0000" ).toTimePoint(), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2021-03-30T10:23:00+0000" );

    nextDate = dateRangeEvent.getNextDate( TimePoint( nextDate->duration() ), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2021-03-30T15:24:00+0000" );

    nextDate = dateRangeEvent.getNextDate( TimePoint( nextDate->duration() ), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2021-03-31T10:23:00+0000" );

    nextDate = dateRangeEvent.getNextDate( TimePoint( nextDate->duration() ), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2021-03-31T15:24:00+0000" );

    nextDate = dateRangeEvent.getNextDate( TimePoint( nextDate->duration() ), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2021-04-01T10:23:00+0000" );

    nextDate = dateRangeEvent.getNextDate( TimePoint( nextDate->duration() ), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2021-04-01T15:24:00+0000" );

    nextDate = dateRangeEvent.getNextDate( Date( "2021-12-31T16:12:44+0000" ).toTimePoint(), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2022-01-01T10:23:00+0000" );

    nextDate = dateRangeEvent.getNextDate( TimePoint( nextDate->duration() ), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2022-01-01T15:24:00+0000" );

    ASSERT_TRUE( dateRangeEvent.setMinuteRange( 24, 26 ) );
    ASSERT_TRUE( dateRangeEvent.setHourRange( 15, 10 ) );

    nextDate = dateRangeEvent.getNextDate( Date( "2021-01-31T09:12:44+0000" ).toTimePoint(), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2021-01-31T10:26:00+0000" );

    nextDate = dateRangeEvent.getNextDate( TimePoint( nextDate->duration() ), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2021-01-31T15:24:00+0000" );

    nextDate = dateRangeEvent.getNextDate( TimePoint( nextDate->duration() ), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2021-02-01T10:26:00+0000" );
}

TEST_F( DataRangeEventTests, monthly ) {
    using namespace GregorianCalendar;
    DateRangeEvent dateRangeEvent;
    ASSERT_TRUE( dateRangeEvent.setMinuteRange( 23, 24 ) );
    ASSERT_TRUE( dateRangeEvent.setHourRange( 10, 15 ) );
    ASSERT_TRUE( dateRangeEvent.setMonthAndDayRanges( { 1, 12 }, { 5, 5 } ) );
    const std::chrono::seconds tzOffset( 0 );

    auto nextDate = dateRangeEvent.getNextDate( Date( "2021-03-01T16:12:44+0000" ).toTimePoint(), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2021-03-05T10:23:00+0000" );

    nextDate = dateRangeEvent.getNextDate( TimePoint( nextDate->duration() ), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2021-03-05T15:24:00+0000" );

    nextDate = dateRangeEvent.getNextDate( TimePoint( nextDate->duration() ), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2021-04-05T10:23:00+0000" );

    nextDate = dateRangeEvent.getNextDate( TimePoint( nextDate->duration() ), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2021-04-05T15:24:00+0000" );

    nextDate = dateRangeEvent.getNextDate( TimePoint( nextDate->duration() ), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2021-05-05T10:23:00+0000" );

    nextDate = dateRangeEvent.getNextDate( TimePoint( nextDate->duration() ), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2021-05-05T15:24:00+0000" );

    // TODO: inverted range
}

TEST_F( DataRangeEventTests, yearly ) {
    using namespace GregorianCalendar;
    DateRangeEvent dateRangeEvent;
    ASSERT_TRUE( dateRangeEvent.setMinuteRange( 23, 24 ) );
    ASSERT_TRUE( dateRangeEvent.setHourRange( 10, 15 ) );
    ASSERT_TRUE( dateRangeEvent.setMonthAndDayRanges( { 10, 10 }, { 5, 5 } ) );
    const std::chrono::seconds tzOffset( 0 );

    auto nextDate = dateRangeEvent.getNextDate( Date( "2021-03-01T16:12:44+0000" ).toTimePoint(), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2021-10-05T10:23:00+0000" );

    nextDate = dateRangeEvent.getNextDate( TimePoint( nextDate->duration() ), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2021-10-05T15:24:00+0000" );

    nextDate = dateRangeEvent.getNextDate( TimePoint( nextDate->duration() ), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2022-10-05T10:23:00+0000" );

    nextDate = dateRangeEvent.getNextDate( TimePoint( nextDate->duration() ), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2022-10-05T15:24:00+0000" );

    nextDate = dateRangeEvent.getNextDate( TimePoint( nextDate->duration() ), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2023-10-05T10:23:00+0000" );

    nextDate = dateRangeEvent.getNextDate( TimePoint( nextDate->duration() ), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2023-10-05T15:24:00+0000" );

    // TODO: inverted range
}

TEST_F( DataRangeEventTests, once ) {
    using namespace GregorianCalendar;
    DateRangeEvent dateRangeEvent;
    ASSERT_TRUE( dateRangeEvent.setMinuteRange( 23, 24 ) );
    ASSERT_TRUE( dateRangeEvent.setHourRange( 10, 15 ) );
    ASSERT_TRUE( dateRangeEvent.setMonthAndDayRanges( { 10, 10 }, { 5, 5 } ) );
    ASSERT_TRUE( dateRangeEvent.setYearRange( 2022, 2022 ) );
    const std::chrono::seconds tzOffset( 0 );

    auto nextDate = dateRangeEvent.getNextDate( Date( "2021-03-01T16:12:44+0000" ).toTimePoint(), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2022-10-05T10:23:00+0000" );

    nextDate = dateRangeEvent.getNextDate( TimePoint( nextDate->duration() ), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2022-10-05T15:24:00+0000" );

    nextDate = dateRangeEvent.getNextDate( TimePoint( nextDate->duration() ), tzOffset );
    ASSERT_FALSE( nextDate );

    // TODO: inverted range
}

TEST_F( DataRangeEventTests, daysRange ) {
    using namespace GregorianCalendar;
    DateRangeEvent dateRangeEvent;
    ASSERT_TRUE( dateRangeEvent.setMonthAndDayRanges( { 1, 12 }, { 5, 20 } ) );
    const std::chrono::seconds tzOffset( 0 );

    auto nextDate = dateRangeEvent.getNextDate( Date( "2021-01-01T16:12:44+0000" ).toTimePoint(), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2021-01-05T00:00:00+0000" );

    nextDate = dateRangeEvent.getNextDate( TimePoint( nextDate->duration() ), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2021-01-20T23:59:00+0000" );

    nextDate = dateRangeEvent.getNextDate( TimePoint( nextDate->duration() ), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2021-02-05T00:00:00+0000" );

    nextDate = dateRangeEvent.getNextDate( TimePoint( nextDate->duration() ), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2021-02-20T23:59:00+0000" );

    nextDate = dateRangeEvent.getNextDate( Date( "2021-12-31T16:12:44+0000" ).toTimePoint(), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2022-01-05T00:00:00+0000" );

    nextDate = dateRangeEvent.getNextDate( TimePoint( nextDate->duration() ), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2022-01-20T23:59:00+0000" );
}

TEST_F( DataRangeEventTests, monthsRange ) {
    using namespace GregorianCalendar;
    DateRangeEvent dateRangeEvent;
    ASSERT_TRUE( dateRangeEvent.setMonthAndDayRanges( { 4, 5 }, { 1, 31 } ) );
    const std::chrono::seconds tzOffset( 0 );

    auto nextDate = dateRangeEvent.getNextDate( Date( "2021-01-01T16:12:44+0000" ).toTimePoint(), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2021-04-01T00:00:00+0000" );

    nextDate = dateRangeEvent.getNextDate( TimePoint( nextDate->duration() ), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2021-05-31T23:59:00+0000" );

    nextDate = dateRangeEvent.getNextDate( TimePoint( nextDate->duration() ), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2022-04-01T00:00:00+0000" );

    nextDate = dateRangeEvent.getNextDate( TimePoint( nextDate->duration() ), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2022-05-31T23:59:00+0000" );

    ASSERT_TRUE( dateRangeEvent.setMonthAndDayRanges( { 2, 11 }, { 1, 31 } ) );

    nextDate = dateRangeEvent.getNextDate( Date( "2021-01-01T16:12:44+0000" ).toTimePoint(), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2021-02-01T00:00:00+0000" );

    nextDate = dateRangeEvent.getNextDate( TimePoint( nextDate->duration() ), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2021-11-30T23:59:00+0000" );

    nextDate = dateRangeEvent.getNextDate( TimePoint( nextDate->duration() ), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2022-02-01T00:00:00+0000" );

    nextDate = dateRangeEvent.getNextDate( TimePoint( nextDate->duration() ), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2022-11-30T23:59:00+0000" );
}

TEST_F( DataRangeEventTests, yearsRange ) {
    using namespace GregorianCalendar;
    DateRangeEvent dateRangeEvent;
    ASSERT_TRUE( dateRangeEvent.setYearRange( 2021, 2025 ) );
    const std::chrono::seconds tzOffset( 0 );

    auto nextDate = dateRangeEvent.getNextDate( Date( "2020-01-01T16:12:44+0000" ).toTimePoint(), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2021-01-01T00:00:00+0000" );

    nextDate = dateRangeEvent.getNextDate( TimePoint( nextDate->duration() ), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2025-12-31T23:59:00+0000" );
}

TEST_F( DataRangeEventTests, oneMonth ) {
    using namespace GregorianCalendar;
    DateRangeEvent dateRangeEvent;
    ASSERT_TRUE( dateRangeEvent.setMinuteRange( 23, 24 ) );
    ASSERT_TRUE( dateRangeEvent.setHourRange( 10, 15 ) );
    ASSERT_TRUE( dateRangeEvent.setMonthAndDayRanges( { 10, 10 }, { 1, 31 } ) );
    const std::chrono::seconds tzOffset( 0 );

    auto nextDate = dateRangeEvent.getNextDate( Date( "2021-03-29T16:12:44+0000" ).toTimePoint(), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2021-10-01T10:23:00+0000" );

    nextDate = dateRangeEvent.getNextDate( TimePoint( nextDate->duration() ), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2021-10-31T15:24:00+0000" );

    nextDate = dateRangeEvent.getNextDate( TimePoint( nextDate->duration() ), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2022-10-01T10:23:00+0000" );

    nextDate = dateRangeEvent.getNextDate( TimePoint( nextDate->duration() ), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2022-10-31T15:24:00+0000" );
}

TEST_F( DataRangeEventTests, oneMonthInYear ) {
    using namespace GregorianCalendar;
    DateRangeEvent dateRangeEvent;
    ASSERT_TRUE( dateRangeEvent.setMinuteRange( 23, 24 ) );
    ASSERT_TRUE( dateRangeEvent.setHourRange( 10, 15 ) );
    ASSERT_TRUE( dateRangeEvent.setMonthAndDayRanges( { 10, 10 }, { 1, 31 } ) );
    ASSERT_TRUE( dateRangeEvent.setYearRange( 2022, 2022 ) );
    const std::chrono::seconds tzOffset( 0 );

    auto nextDate = dateRangeEvent.getNextDate( Date( "2021-03-29T16:12:44+0000" ).toTimePoint(), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2022-10-01T10:23:00+0000" );

    nextDate = dateRangeEvent.getNextDate( TimePoint( nextDate->duration() ), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2022-10-31T15:24:00+0000" );

    nextDate = dateRangeEvent.getNextDate( TimePoint( nextDate->duration() ), tzOffset );
    ASSERT_FALSE( nextDate );
}

TEST_F( DataRangeEventTests, oneYear1 ) {
    using namespace GregorianCalendar;
    DateRangeEvent dateRangeEvent;
    ASSERT_TRUE( dateRangeEvent.setMinuteRange( 23, 24 ) );
    ASSERT_TRUE( dateRangeEvent.setHourRange( 10, 15 ) );
    ASSERT_TRUE( dateRangeEvent.setYearRange( 2022, 2022 ) );
    const std::chrono::seconds tzOffset( 0 );

    auto nextDate = dateRangeEvent.getNextDate( Date( "2021-03-29T16:12:44+0000" ).toTimePoint(), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2022-01-01T10:23:00+0000" );

    nextDate = dateRangeEvent.getNextDate( TimePoint( nextDate->duration() ), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2022-12-31T15:24:00+0000" );

    nextDate = dateRangeEvent.getNextDate( TimePoint( nextDate->duration() ), tzOffset );
    ASSERT_FALSE( nextDate );
}

TEST_F( DataRangeEventTests, oneYear2 ) {
    using namespace GregorianCalendar;
    DateRangeEvent dateRangeEvent;
    ASSERT_TRUE( dateRangeEvent.setMinuteRange( 23, 24 ) );
    ASSERT_TRUE( dateRangeEvent.setHourRange( 10, 15 ) );
    ASSERT_TRUE( dateRangeEvent.setMonthAndDayRanges( { 1, 12 }, { 5, 5 } ) );
    ASSERT_TRUE( dateRangeEvent.setYearRange( 2022, 2022 ) );
    const std::chrono::seconds tzOffset( 0 );

    auto nextDate = dateRangeEvent.getNextDate( Date( "2021-03-01T16:12:44+0000" ).toTimePoint(), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2022-01-05T10:23:00+0000" );

    nextDate = dateRangeEvent.getNextDate( TimePoint( nextDate->duration() ), tzOffset );
    ASSERT_TRUE( nextDate );
    EXPECT_EQ( Date( TimePoint( nextDate->duration() ) ).toString(), "2022-12-05T15:24:00+0000" );

    nextDate = dateRangeEvent.getNextDate( TimePoint( nextDate->duration() ), tzOffset );
    ASSERT_FALSE( nextDate );
}

}  // namespace UT
}  // namespace SceneEngine
