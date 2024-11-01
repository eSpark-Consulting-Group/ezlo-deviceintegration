#include "scenes_manager/scenes_engine/date_scheduler/date_scheduler.h"
#include "scenes_manager/scenes_engine/events/date_event.h"
#include "scenes_manager/scenes_engine/events/daily_event.h"
#include "scenes_manager/scenes_engine/events/weekly_event.h"
#include "scenes_manager/scenes_engine/events/monthly_event.h"
#include "scenes_manager/scenes_engine/events/once_event.h"
#include "scenes_manager/scenes_engine/events/sun_state_event.h"
#include "scenes_manager/scenes_engine/events/interval_event.h"
#include "scenes_manager/scenes_engine/helpers/test_clock.h"

#include "base_utils/gregorian_calendar.h"
#include "base_utils/timeutils.h"
#include "common_utils/cpp14.h"
#include "json_utils/json_wrapper.h"
#include "log_utils/ezlo_log.h"
#include "scenes_manager/scenes_engine/helpers/clock_interface.h"

#include <gtest/gtest.h>

/*

 TEST PLAN

DONE    1. Date event tests
DONE        1.1 daily event
DONE            1.1.1 daily event UTC
DONE            1.1.2 daily event with offset
DONE        1.2 weekly event
DONE            1.2.1 weekly event, offset 0
DONE            1.2.2 weekly event, positive offset
DONE        1.3 monthly event
DONE    2. Date scheduler tests
DONE        2.1 default state
DONE        2.2 singe daily event activated
DONE        2.3 single day weekly events
DONE        2.4 different day weekly events
DONE        2.5 date recalculation on big time shifts
DONE        2.6 events are not activated by default
DONE        2.7 event activated once regardless of passed time
DONE        2.8 once event
DONE        2.9 event not removed on activation
DONE    3. Date events with same activation time
DONE        3.1 isDate events
DONE        3.2 isOnce events
DONE        3.3 isDate and isOnce
DONE        3.4 isSunState events
DONE        3.5 isInterval events
DONE    4. DateScheduler::EventEntry: DST test for operator <()


 */

namespace SceneEngine {
namespace UT {

const std::set< DateEvent::TimeType > timeArray{ { 10, 30 }, { 12, 01 }, { 03, 00 } };
const std::set< DateEventBase::Day > daysArray{ 1, 23, 17, 31 };
const std::set< DateEventBase::Day > weekdaysArray{ 5, 4, 1, 3 };
const std::set< DateEventBase::Week > weeksArray{ 5, 4, 1, 3, -1 };

const GregorianCalendar::ClockDuration defaultStartTime{ 1738301208000000000ll };  // 2025-01-31T05:26:48+0000
const std::chrono::seconds defaultStartTimeOffset{ 0 };

using namespace GregorianCalendar;

class DateEventSuite : public testing::Test {
public:
    static GregorianCalendar::TimePoint toLocalTime( const GregorianCalendar::Date& date );

    using DailyEventCheckPred = std::function< void( Date&, const std::string&, int ) >;
    static DailyEventCheckPred dailyEventChecker( DateEventBase& dateEvent );

    using WeeklyEventCheckPred = std::function< void( Date&, WeeklyEvent::Weekday, const std::string&, int ) >;
    static WeeklyEventCheckPred weeklyEventChecker( DateEventBase& dateEvent );
};

/** Converts UTC time point to localtime time point: utc shifted to timezone offset, like IClock::localTime() does */
GregorianCalendar::TimePoint DateEventSuite::toLocalTime( const GregorianCalendar::Date& date ) {
    using namespace GregorianCalendar;
    return static_cast< TimePoint >( Date::toLocalTime( date.toClockDuration(), date.getTimeZoneOffset() ) );
}

DateEventSuite::DailyEventCheckPred DateEventSuite::dailyEventChecker( DateEventBase& dateEvent ) {
    auto checkEvent = [&dateEvent]( Date& date, const std::string& expectedTime, int codeLine ) {
        auto localTime = toLocalTime( date );
        auto timezone = date.getTimeZoneOffset();
        auto nextDate = dateEvent.getNextDate( localTime, timezone );
        date = Date::fromLocalTime( nextDate->duration(), nextDate->offset() );
        ASSERT_TRUE( nextDate ) << __FILE__ << ":" << codeLine;
        EXPECT_EQ( date.toString(), expectedTime ) << __FILE__ << ":" << codeLine;
    };

    return checkEvent;
}

DateEventSuite::WeeklyEventCheckPred DateEventSuite::weeklyEventChecker( DateEventBase& dateEvent ) {
    auto checkWeeklyEvent =
        [&dateEvent]( Date& date, WeeklyEvent::Weekday weekday, const std::string& expectedTime, int codeLine ) {
            auto localTime = toLocalTime( date );
            auto nextDate = dateEvent.getNextDate( localTime, date.getTimeZoneOffset() );
            date = Date::fromLocalTime( nextDate->duration(), nextDate->offset() );
            ASSERT_TRUE( nextDate ) << __FILE__ << ":" << codeLine;
            EXPECT_EQ( date.getWeekday(), weekday ) << __FILE__ << ":" << codeLine;
            EXPECT_EQ( date.toString(), expectedTime ) << __FILE__ << ":" << codeLine;
        };

    return checkWeeklyEvent;
}

class DateSchedulerSuite : public testing::Test {
public:
    void SetUp() override {
        m_dateScheduler.setTime( defaultStartTime, defaultStartTimeOffset );
        m_dateScheduler.enableRecalculationOnBigTimeShift( false );
    }
    DateScheduler& dateScheduler() {
        return m_dateScheduler;
    }

    static std::string makeError( int line ) {
        return std::string{ __FILE__ }.append( ":" ).append( std::to_string( line ) );
    }
    void checkActivatedEvents( const std::vector< DateEventPtr >& expectedActivation, int line ) {
        auto activatedEvents = dateScheduler().collectActivatedEvents();
        EXPECT_EQ( activatedEvents.size(), expectedActivation.size() ) << makeError( line );
        EXPECT_EQ( activatedEvents, expectedActivation ) << makeError( line );
    }

private:
    DateScheduler m_dateScheduler;
};

TEST_F( DateEventSuite, dailyEvent ) {  // 1.1.1
    using namespace GregorianCalendar;

    Date date{ TimePoint( defaultStartTime ) };
    DailyEvent dailyEvent( timeArray );

    auto checkNextDailyEvent = dailyEventChecker( dailyEvent );

    checkNextDailyEvent( date, "2025-01-31T10:30:00+0000", __LINE__ );
    checkNextDailyEvent( date, "2025-01-31T12:01:00+0000", __LINE__ );
    checkNextDailyEvent( date, "2025-02-01T03:00:00+0000", __LINE__ );
}

TEST_F( DateEventSuite, dailyEventWithOffset ) {  // 1.1.2
    using namespace GregorianCalendar;
    using namespace std::chrono;

    Date date{ "2023-11-07T23:00:00+0200" };
    DailyEvent dailyEvent( timeArray );

    auto checkNextDailyEvent = dailyEventChecker( dailyEvent );

    checkNextDailyEvent( date, "2023-11-08T03:00:00+0200", __LINE__ );
    checkNextDailyEvent( date, "2023-11-08T10:30:00+0200", __LINE__ );
    checkNextDailyEvent( date, "2023-11-08T12:01:00+0200", __LINE__ );
}

TEST_F( DateEventSuite, dailyEventWithOffsetDST ) {  // 1.1.1
    using namespace GregorianCalendar;

    Date date{ "2023-10-28T00:00:00+0300" };  // DTS change: DST -> standard time
    const std::set< DateEvent::TimeType > dstTimeArray{ { 2, 10 }, { 3, 10 }, { 03, 59 }, { 5, 10 } };

    DailyEvent dailyEvent( dstTimeArray );

    auto checkNextDailyEvent = dailyEventChecker( dailyEvent );

    {
        checkNextDailyEvent( date, "2023-10-28T02:10:00+0300", __LINE__ );
        checkNextDailyEvent( date, "2023-10-28T03:10:00+0300", __LINE__ );
        checkNextDailyEvent( date, "2023-10-28T03:59:00+0300", __LINE__ );

        date = Date( "2023-10-28T03:00:00+0200" );  // Emulate local time transition: DST finished, shift -1 hour
        checkNextDailyEvent( date, "2023-10-28T03:10:00+0200", __LINE__ );
        checkNextDailyEvent( date, "2023-10-28T03:59:00+0200", __LINE__ );
        checkNextDailyEvent( date, "2023-10-28T05:10:00+0200", __LINE__ );
    }

    {
        date = Date( "2023-03-26T02:00:00+0200" );
        checkNextDailyEvent( date, "2023-03-26T02:10:00+0200", __LINE__ );
        date = Date( "2023-03-26T04:00:00+0300" );  // Emulate local time transition: DST started, shift +1 hour
        checkNextDailyEvent( date, "2023-03-26T05:10:00+0300", __LINE__ );
    }
}

TEST_F( DateEventSuite, weeklyEvent ) {  // 1.2.1
    using namespace GregorianCalendar;

    Date date( "2025-01-31T05:26:48+0000" );  // FRI
    WeeklyEvent weeklyEvent( timeArray, weekdaysArray );

    auto checkNextWeeklyEvent = weeklyEventChecker( weeklyEvent );

    checkNextWeeklyEvent( date, WeeklyEvent::Weekday::FRI, "2025-01-31T10:30:00+0000", __LINE__ );
    checkNextWeeklyEvent( date, WeeklyEvent::Weekday::FRI, "2025-01-31T12:01:00+0000", __LINE__ );
    checkNextWeeklyEvent( date, WeeklyEvent::Weekday::MON, "2025-02-03T03:00:00+0000", __LINE__ );
    checkNextWeeklyEvent( date, WeeklyEvent::Weekday::MON, "2025-02-03T10:30:00+0000", __LINE__ );
    checkNextWeeklyEvent( date, WeeklyEvent::Weekday::MON, "2025-02-03T12:01:00+0000", __LINE__ );
    checkNextWeeklyEvent( date, WeeklyEvent::Weekday::WED, "2025-02-05T03:00:00+0000", __LINE__ );
}

TEST_F( DateEventSuite, weeklyEventWithOffset ) {  // 1.2.2
    using namespace GregorianCalendar;

    // Europe/Kiev, no DST, timezone offset +0200
    Date date( "2023-11-07T00:00:00+0200" );  // Tuesday
    const std::set< DateEventBase::Day > weekdaysArray{
        WeeklyEvent::Weekday::THU, WeeklyEvent::Weekday::MON, WeeklyEvent::Weekday::WED
    };

    WeeklyEvent weeklyEvent( timeArray, weekdaysArray );
    auto checkNextWeeklyEvent = weeklyEventChecker( weeklyEvent );

    // TUE is not in weekdays -> WED -> 3:00 -> 10:30 -> 12:01
    checkNextWeeklyEvent( date, WeeklyEvent::Weekday::WED, "2023-11-08T03:00:00+0200", __LINE__ );
    checkNextWeeklyEvent( date, WeeklyEvent::Weekday::WED, "2023-11-08T10:30:00+0200", __LINE__ );
    checkNextWeeklyEvent( date, WeeklyEvent::Weekday::WED, "2023-11-08T12:01:00+0200", __LINE__ );

    // THU -> 3:00 -> 10:30 -> 12:01
    checkNextWeeklyEvent( date, WeeklyEvent::Weekday::THU, "2023-11-09T03:00:00+0200", __LINE__ );
    checkNextWeeklyEvent( date, WeeklyEvent::Weekday::THU, "2023-11-09T10:30:00+0200", __LINE__ );
    checkNextWeeklyEvent( date, WeeklyEvent::Weekday::THU, "2023-11-09T12:01:00+0200", __LINE__ );

    // Mon -> 3:00 -> 10:30 -> 12:01
    checkNextWeeklyEvent( date, WeeklyEvent::Weekday::MON, "2023-11-13T03:00:00+0200", __LINE__ );
    checkNextWeeklyEvent( date, WeeklyEvent::Weekday::MON, "2023-11-13T10:30:00+0200", __LINE__ );
    checkNextWeeklyEvent( date, WeeklyEvent::Weekday::MON, "2023-11-13T12:01:00+0200", __LINE__ );
}

TEST_F( DateEventSuite, monthlyEvent ) {  // 1.3
    using namespace GregorianCalendar;

    Date date{ TimePoint( defaultStartTime ) };
    MonthlyEvent monthlyEvent( timeArray, daysArray );

    auto checkNextDailyEvent = dailyEventChecker( monthlyEvent );

    checkNextDailyEvent( date, "2025-01-31T10:30:00+0000", __LINE__ );
    checkNextDailyEvent( date, "2025-01-31T12:01:00+0000", __LINE__ );
    checkNextDailyEvent( date, "2025-02-01T03:00:00+0000", __LINE__ );
    checkNextDailyEvent( date, "2025-02-01T10:30:00+0000", __LINE__ );
    checkNextDailyEvent( date, "2025-02-01T12:01:00+0000", __LINE__ );
    checkNextDailyEvent( date, "2025-02-17T03:00:00+0000", __LINE__ );
    checkNextDailyEvent( date, "2025-02-17T10:30:00+0000", __LINE__ );
    checkNextDailyEvent( date, "2025-02-17T12:01:00+0000", __LINE__ );
    checkNextDailyEvent( date, "2025-02-23T03:00:00+0000", __LINE__ );
    checkNextDailyEvent( date, "2025-02-23T10:30:00+0000", __LINE__ );
    checkNextDailyEvent( date, "2025-02-23T12:01:00+0000", __LINE__ );
    checkNextDailyEvent( date, "2025-03-01T03:00:00+0000", __LINE__ );
    checkNextDailyEvent( date, "2025-03-01T10:30:00+0000", __LINE__ );
    checkNextDailyEvent( date, "2025-03-01T12:01:00+0000", __LINE__ );
}

TEST_F( DateSchedulerSuite, defaultState ) {  // 2.1
    EXPECT_TRUE( dateScheduler().empty() );
    EXPECT_TRUE( dateScheduler().collectActivatedEvents().empty() );

    auto now = DateScheduler::ClockDuration::max();
    dateScheduler().setTime( now, std::chrono::seconds() );
    EXPECT_TRUE( dateScheduler().collectActivatedEvents().empty() );
}

TEST_F( DateSchedulerSuite, singeDailyEventActivated ) {  // 2.2
    auto now = defaultStartTime;
    auto offset = defaultStartTimeOffset;
    dateScheduler().setTime( now, offset );

    auto testEvent = std::make_shared< DailyEvent >( timeArray );
    dateScheduler().push( testEvent );

    for( int i = 0; i < timeArray.size(); ++i ) {
        auto nextDate = testEvent->getNextDate( GregorianCalendar::TimePoint( now ), offset );

        EXPECT_TRUE( nextDate != nullptr );
        now = nextDate->duration();
        offset = nextDate->offset();

        dateScheduler().setTime( nextDate->duration(), nextDate->offset() );
        checkActivatedEvents( { testEvent }, __LINE__ );
    }
}

TEST_F( DateSchedulerSuite, singleDayWeeklyEvents ) {  // 2.3
    LocalTimePoint nextDate( defaultStartTime, defaultStartTimeOffset );
    dateScheduler().setTime( nextDate.duration(), nextDate.offset() );

    const Json weekdays{ 2 };
    auto weeklyEvent1 = std::make_shared< WeeklyEvent >( WeeklyEvent::TimeList{ { 10, 00 }, { 12, 00 } }, weekdays );
    auto weeklyEvent2 = std::make_shared< WeeklyEvent >( WeeklyEvent::TimeList{ { 11, 00 }, { 13, 00 } }, weekdays );
    dateScheduler().push( weeklyEvent1 );
    dateScheduler().push( weeklyEvent2 );

    // all activations during a week
    nextDate = *weeklyEvent1->getNextDate( GregorianCalendar::TimePoint( nextDate.duration() ), nextDate.offset() );
    dateScheduler().setTime( nextDate.duration(), nextDate.offset() );
    checkActivatedEvents( { weeklyEvent1 }, __LINE__ );

    nextDate = *weeklyEvent2->getNextDate( GregorianCalendar::TimePoint( nextDate.duration() ), nextDate.offset() );
    dateScheduler().setTime( nextDate.duration(), nextDate.offset() );
    checkActivatedEvents( { weeklyEvent2 }, __LINE__ );

    nextDate = *weeklyEvent2->getNextDate( GregorianCalendar::TimePoint( nextDate.duration() ), nextDate.offset() );
    dateScheduler().setTime( nextDate.duration(), nextDate.offset() );
    checkActivatedEvents( { weeklyEvent1, weeklyEvent2 }, __LINE__ );

    // activation on next week
    nextDate = *weeklyEvent1->getNextDate( GregorianCalendar::TimePoint( nextDate.duration() ), nextDate.offset() );
    dateScheduler().setTime( nextDate.duration(), nextDate.offset() );
    checkActivatedEvents( { weeklyEvent1 }, __LINE__ );
}

TEST_F( DateSchedulerSuite, differentDayWeeklyEvents ) {  // 2.4
    using Day = WeeklyEvent::Day;

    LocalTimePoint nextDate( defaultStartTime, defaultStartTimeOffset );

    dateScheduler().setTime( nextDate.duration(), nextDate.offset() );

    auto eventActivationTime1 = WeeklyEvent::TimeList{ { 10, 00 }, { 12, 00 } };
    auto eventActivationTime2 = WeeklyEvent::TimeList{ { 11, 00 }, { 11, 00 } };
    auto weeklyEvent1 = std::make_shared< WeeklyEvent >( eventActivationTime1, std::set< Day >{ 1 } );
    weeklyEvent1->setEventId( "weeklyEvent1" );
    auto weeklyEvent2 = std::make_shared< WeeklyEvent >( eventActivationTime2, std::set< Day >{ 3 } );
    weeklyEvent2->setEventId( "weeklyEvent2" );
    dateScheduler().push( weeklyEvent1 );
    dateScheduler().push( weeklyEvent2 );

    // all activations during a week
    for( int i = 0; i < eventActivationTime1.size(); ++i ) {
        nextDate = *weeklyEvent1->getNextDate( GregorianCalendar::TimePoint( nextDate.duration() ), nextDate.offset() );

        dateScheduler().setTime( nextDate.duration(), nextDate.offset() );
        checkActivatedEvents( { weeklyEvent1 }, __LINE__ );
    }
    for( int i = 0; i < eventActivationTime2.size(); ++i ) {
        nextDate = *weeklyEvent2->getNextDate( GregorianCalendar::TimePoint( nextDate.duration() ), nextDate.offset() );
        dateScheduler().setTime( nextDate.duration(), nextDate.offset() );
        checkActivatedEvents( { weeklyEvent2 }, __LINE__ );
    }

    // activation on next week
    nextDate = *weeklyEvent1->getNextDate( GregorianCalendar::TimePoint( nextDate.duration() ), nextDate.offset() );
    dateScheduler().setTime( nextDate.duration(), nextDate.offset() );
    checkActivatedEvents( { weeklyEvent1 }, __LINE__ );
}

TEST_F( DateSchedulerSuite, dateRecalculationOnBigTimeShifts ) {  // 2.5
    auto now = defaultStartTime;
    auto offset = defaultStartTimeOffset;
    dateScheduler().setTime( now, offset );
    dateScheduler().enableRecalculationOnBigTimeShift( true );

    auto testEvent = std::make_shared< DailyEvent >( timeArray );
    dateScheduler().push( testEvent );

    // event not activated due to big time shift
    auto nextDate = testEvent->getNextDate( GregorianCalendar::TimePoint( now ), offset );
    dateScheduler().setTime( nextDate->duration(), nextDate->offset() );
    checkActivatedEvents( {}, __LINE__ );

    // event is activated in regular flow
    dateScheduler().enableRecalculationOnBigTimeShift( false );
    nextDate = testEvent->getNextDate( GregorianCalendar::TimePoint( nextDate->duration() ), nextDate->offset() );
    dateScheduler().setTime( nextDate->duration(), nextDate->offset() );
    checkActivatedEvents( { testEvent }, __LINE__ );
}

TEST_F( DateSchedulerSuite, eventsAreNotActivatedByDefault ) {  // 2.6
    auto now = defaultStartTime;
    dateScheduler().setTime( now, std::chrono::seconds() );

    auto testEvent = std::make_shared< DailyEvent >( timeArray );
    dateScheduler().push( testEvent );
    EXPECT_TRUE( dateScheduler().collectActivatedEvents().empty() );
}

TEST_F( DateSchedulerSuite, eventActivatedOnceRegardlessOfPassedTime ) {  // 2.7
    auto now = defaultStartTime;
    auto offset = defaultStartTimeOffset;
    dateScheduler().setTime( defaultStartTime, offset );

    auto testEvent = std::make_shared< DailyEvent >( timeArray );
    dateScheduler().push( testEvent );

    now = testEvent->getNextDate( GregorianCalendar::TimePoint( now ), offset )->duration();
    now = testEvent->getNextDate( GregorianCalendar::TimePoint( now ), offset )->duration();
    dateScheduler().setTime( now, offset );
    checkActivatedEvents( { testEvent }, __LINE__ );
}

TEST_F( DateSchedulerSuite, onceEvent ) {  // 2.8
    auto now = GregorianCalendar::Date( "2025-01-31T05:26:48" ).toClockDuration();
    dateScheduler().setTime( now, std::chrono::seconds( 0 ) );

    GregorianCalendar::Date date1;
    GregorianCalendar::Date date2;
    ASSERT_TRUE( date1.setDate( 2026, 5, 15 ) && date1.setTime( 12, 10, 0 ) );
    ASSERT_TRUE( date2.setDate( 2026, 8, 17 ) && date2.setTime( 20, 15, 0 ) );

    auto onceEvent1 = std::make_shared< OnceEvent >( date1 );
    auto onceEvent2 = std::make_shared< OnceEvent >( date2 );

    dateScheduler().push( onceEvent1 );
    dateScheduler().push( onceEvent2 );

    now = onceEvent1->getNextDate( GregorianCalendar::TimePoint( now ), std::chrono::seconds( 0 ) )->duration();
    dateScheduler().setTime( now, std::chrono::seconds() );
    checkActivatedEvents( { onceEvent1 }, __LINE__ );

    now = onceEvent2->getNextDate( GregorianCalendar::TimePoint( now ), std::chrono::seconds( 0 ) )->duration();
    dateScheduler().setTime( now, std::chrono::seconds() );
    checkActivatedEvents( { onceEvent2 }, __LINE__ );
}

TEST_F( DateSchedulerSuite, eventNotRemovedOnActivation ) {  // 2.9

    dateScheduler().setTime( defaultStartTime, defaultStartTimeOffset );

    auto testEvent = std::make_shared< DailyEvent >( timeArray );
    dateScheduler().push( testEvent );

    auto nextDate = *testEvent->getNextDate( GregorianCalendar::TimePoint( defaultStartTime ), defaultStartTimeOffset );
    dateScheduler().setTime( nextDate.duration(), nextDate.offset() );
    EXPECT_EQ( dateScheduler().collectActivatedEvents().size(), 1 );
    EXPECT_FALSE( dateScheduler().empty() );
}

TEST_F( DateSchedulerSuite, isDateEventsWithSameActivationTime ) {  // 3.1
    dateScheduler().setTime( defaultStartTime, std::chrono::seconds( 0 ) );

    const auto timeList = DailyEvent::TimeList{ { 10, 30 } };
    auto event1 = std::make_shared< DailyEvent >( timeList );
    auto event2 = std::make_shared< DailyEvent >( timeList );
    dateScheduler().push( event1 );
    dateScheduler().push( event2 );

    auto nextDate = *event1->getNextDate( GregorianCalendar::TimePoint( defaultStartTime ), std::chrono::seconds( 0 ) );
    dateScheduler().setTime( nextDate.duration(), nextDate.offset() );
    checkActivatedEvents( { event1, event2 }, __LINE__ );
}

TEST_F( DateSchedulerSuite, isOnceEventsWithSameActivationTime ) {  // 3.2
    dateScheduler().setTime( defaultStartTime, std::chrono::seconds() );

    GregorianCalendar::Date date;
    ASSERT_TRUE( date.setDate( 2026, 5, 15 ) && date.setTime( 12, 10, 0 ) );

    auto event1 = std::make_shared< OnceEvent >( date );
    auto event2 = std::make_shared< OnceEvent >( date );

    dateScheduler().push( event1 );
    dateScheduler().push( event2 );

    auto nextDate = *event1->getNextDate( GregorianCalendar::TimePoint( defaultStartTime ), std::chrono::seconds( 0 ) );
    dateScheduler().setTime( nextDate.duration(), nextDate.offset() );
    checkActivatedEvents( { event1, event2 }, __LINE__ );
}

TEST_F( DateSchedulerSuite, dailyAndOnceEventsWithSameActivationTime ) {  // 3.3
    using namespace GregorianCalendar;

    dateScheduler().setTime( defaultStartTime, std::chrono::seconds() );

    auto dailyEvent = std::make_shared< DailyEvent >( DailyEvent::TimeList{ { 10, 30 } } );

    Date date( TimePoint{ defaultStartTime } );
    ASSERT_TRUE( date.setTime( 10, 30, 0 ) );
    auto onceEvent = std::make_shared< OnceEvent >( date );

    dateScheduler().push( dailyEvent );
    dateScheduler().push( onceEvent );

    auto nextDate =
        *dailyEvent->getNextDate( GregorianCalendar::TimePoint( defaultStartTime ), date.getTimeZoneOffset() );
    dateScheduler().setTime( nextDate.duration(), nextDate.offset() );
    checkActivatedEvents( { dailyEvent, onceEvent }, __LINE__ );
}

TEST_F( DateSchedulerSuite, isSunStateEventsWithSameActivationTime ) {  // 3.4
    dateScheduler().setTime( defaultStartTime, std::chrono::seconds() );

    auto stateType = SunStateEvent::State::SUN_SET;
    auto offsetType = SunStateEvent::OffsetType::IN_TIME;
    auto timeZoneOffset = std::chrono::hours( 0 );
    auto geoPositionGetter = []() { return HubPlatform::CoreDRemoteInterface::GeoPosition{ 0.0f, 0.0f }; };

    auto clocks = std::make_shared< TestClock >( std::chrono::seconds( 0 ), timeZoneOffset );
    auto clockGetter = [clocks]() mutable -> IClock& { return *clocks.get(); };

    auto event1 = std::make_shared< SunStateEvent >( stateType, offsetType, geoPositionGetter, clockGetter );
    auto event2 = std::make_shared< SunStateEvent >( stateType, offsetType, geoPositionGetter, clockGetter );
    ASSERT_FALSE( event1->isRangeEvent() );
    ASSERT_FALSE( event2->isRangeEvent() );

    dateScheduler().push( std::static_pointer_cast< DateEventBase >( event1 ) );
    dateScheduler().push( std::static_pointer_cast< DateEventBase >( event2 ) );

    auto nextDate = event1->getNextDate( GregorianCalendar::TimePoint( defaultStartTime ), defaultStartTimeOffset );
    ASSERT_NE( nextDate, nullptr );

    dateScheduler().setTime( nextDate->duration(), nextDate->offset() );
    checkActivatedEvents( { event1, event2 }, __LINE__ );
}

TEST_F( DateSchedulerSuite, isIntervalEventsWithSameActivationTime ) {  // 3.5
    dateScheduler().setTime( defaultStartTime, defaultStartTimeOffset );

    const Json intervalSettings( "2s" );
    auto event1 = std::make_shared< IntervalEvent >( intervalSettings );
    auto event2 = std::make_shared< IntervalEvent >( intervalSettings );
    dateScheduler().push( event1 );
    dateScheduler().push( event2 );

    auto nextDate = *event1->getNextDate( GregorianCalendar::TimePoint( defaultStartTime ), defaultStartTimeOffset );
    dateScheduler().setTime( nextDate.duration(), nextDate.offset() );
    checkActivatedEvents( { event1, event2 }, __LINE__ );
}

TEST_F( DateSchedulerSuite, EventEntryOperatorLess ) /* 4 */ {
    using namespace std::chrono;
    using namespace GregorianCalendar;
    DateScheduler::EventEntry left, right;

    EXPECT_FALSE( left < right ) << "Empty events should match";

    left.m_nextEvent.setLocalTime( system_clock::now().time_since_epoch(), hours( 3 ) );
    right.m_nextEvent.setLocalTime( system_clock::now().time_since_epoch(), hours( 2 ) );
    EXPECT_TRUE( left < right );

    left.m_nextEvent.setLocalTime( system_clock::now().time_since_epoch(), hours( 3 ) );
    right.m_nextEvent.setLocalTime( system_clock::now().time_since_epoch() - hours( 1 ), hours( 2 ) );
    EXPECT_FALSE( left < right );

    right.m_nextEvent.setLocalTime( system_clock::now().time_since_epoch() - hours( 1 ) + seconds( 1 ), hours( 2 ) );
    EXPECT_TRUE( left < right );
}

}  // namespace UT
}  // namespace SceneEngine
