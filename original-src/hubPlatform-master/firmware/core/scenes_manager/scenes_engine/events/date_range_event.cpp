#include "scenes_manager/scenes_engine/events/date_range_event.h"
#include "common_utils/cpp14.h"
#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"

namespace SceneEngine {

DateRangeEvent::DateRangeEvent()
: m_yearRange( 1, std::numeric_limits< Year >::max() )
, m_monthRange( 1, 12 )
, m_dayRange( 1, 31 )
, m_hourRange( 0, 23 )
, m_minuteRange( 0, 59 )
, m_scheduleType( ScheduleType::Daily ) {
    setScheduleType();
    printHash();
}

bool DateRangeEvent::setYearRange( Year begin, Year end ) {
    if( begin < 1 ) {
        return false;
    }
    if( end < 1 ) {
        return false;
    }
    m_yearRange = { begin, end };
    setScheduleType();
    printHash();
    return true;
}

bool DateRangeEvent::setMonthAndDayRanges( const MonthRange& monthRange, const DayRange& dayRange ) {
    if( monthRange.first < 1 || monthRange.first > 12 ) {
        return false;
    }
    if( monthRange.second < 1 || monthRange.second > 12 ) {
        return false;
    }
    if( dayRange.first < 1 ) {
        return false;
    }
    if( dayRange.second < 1 ) {
        return false;
    }
    m_monthRange = monthRange;
    m_dayRange = dayRange;
    auto daysInMonth = GregorianCalendar::Date::getDaysInMonth( monthRange.first );
    if( dayRange.first > daysInMonth ) {
        m_dayRange.first = daysInMonth;
    }
    daysInMonth = GregorianCalendar::Date::getDaysInMonth( monthRange.second );
    if( dayRange.second > daysInMonth ) {
        m_dayRange.second = daysInMonth;
    }
    setScheduleType();
    printHash();
    return true;
}

bool DateRangeEvent::setHourRange( Hour begin, Hour end ) {
    if( begin > 23 ) {
        return false;
    }
    if( end > 23 ) {
        return false;
    }
    m_hourRange = { begin, end };
    setScheduleType();
    return true;
}

bool DateRangeEvent::setMinuteRange( Minute begin, Minute end ) {
    if( begin > 59 ) {
        return false;
    }
    if( end > 59 ) {
        return false;
    }
    m_minuteRange = { begin, end };
    setScheduleType();
    printHash();
    return true;
}

std::string DateRangeEvent::getMethodName() const {
    return Model::Resources::MethodNames::isDateRange;
}

DateEventBase::OptionalClock DateRangeEvent::getNextDate( const GregorianCalendar::TimePoint& localNow,
                                                          TimeZoneOffset offset ) const {
    using namespace GregorianCalendar;
    GregorianCalendar::Date date = Date::fromLocalTime( localNow, offset );
    switch( m_scheduleType ) {
    case ScheduleType::Daily: {
        if( isInTimeRange( date ) ) {
            date.setHour( m_hourRange.second );
            date.setMinute( m_minuteRange.second );
        }
        else {
            date.setHour( m_hourRange.first );
            date.setMinute( m_minuteRange.first );
        }
        date.setSecond( 0 );
        date.setMillisecond( 0 );
        // utc ws localtime
        if( date.toClockDuration() <= localNow.time_since_epoch() ) {
            date.incrementDay();
        }
        break;
    }
    case ScheduleType::Monthly: {
        if( isInDayRange( date ) ) {
            Day day = m_dayRange.second > date.getMaxMonthDay() ? date.getMaxMonthDay() : m_dayRange.second;
            date.setDay( day );
            date.setHour( m_hourRange.second );
            date.setMinute( m_minuteRange.second );
        }
        else {
            Day day = m_dayRange.first > date.getMaxMonthDay() ? date.getMaxMonthDay() : m_dayRange.first;
            date.setDay( day );
            date.setHour( m_hourRange.first );
            date.setMinute( m_minuteRange.first );
        }
        date.setSecond( 0 );
        date.setMillisecond( 0 );
        if( date.toClockDuration() <= localNow.time_since_epoch() ) {
            date.incrementMonth();
        }
        break;
    }
    case ScheduleType::Yearly: {
        if( isInMonthRange( date ) ) {
            date.setMonth( m_monthRange.second );
            date.setDay( m_dayRange.second );
            date.setHour( m_hourRange.second );
            date.setMinute( m_minuteRange.second );
        }
        else {
            date.setMonth( m_monthRange.first );
            date.setDay( m_dayRange.first );
            date.setHour( m_hourRange.first );
            date.setMinute( m_minuteRange.first );
        }
        date.setSecond( 0 );
        date.setMillisecond( 0 );
        if( date.toClockDuration() <= localNow.time_since_epoch() ) {
            date.incrementYear();
        }
        break;
    }
    case ScheduleType::Years: {
        if( isInYearRange( date ) ) {
            date.setYear( m_yearRange.second );
            date.setMonth( m_monthRange.second );
            date.setDay( m_dayRange.second );
            date.setHour( m_hourRange.second );
            date.setMinute( m_minuteRange.second );
        }
        else {
            date.setYear( m_yearRange.first );
            date.setMonth( m_monthRange.first );
            date.setDay( m_dayRange.first );
            date.setHour( m_hourRange.first );
            date.setMinute( m_minuteRange.first );
        }
        date.setSecond( 0 );
        date.setMillisecond( 0 );
        break;
    }
    }
    if( auto nextDate = getNextDate( localNow, offset, date ) ) {
        return nextDate;
    }
    LOGGER_TRACE( "next date not exist" );
    return nullptr;
}

bool DateRangeEvent::isTimePointInRange( const GregorianCalendar::TimePoint& localNow,
                                         TimeZoneOffset offset,
                                         bool verbose ) const {
    using namespace GregorianCalendar;
    Date date = Date::fromLocalTime( localNow, offset );

    switch( m_scheduleType ) {
    case ScheduleType::Daily:
        return isInTimeRange( date );
    case ScheduleType::Monthly:
        return isInDayRange( date );
    case ScheduleType::Yearly:
        return isInMonthRange( date );
    case ScheduleType::Years:
        return isInYearRange( date );
    }
    return false;
}

DateRangeEvent::ScheduleType DateRangeEvent::getScheduleType() const {
    if( !isForeverRange() ) {
        return ScheduleType::Years;
    }
    if( !isAllYearRange() ) {
        return ScheduleType::Yearly;
    }
    if( !isAllMonthRange() ) {
        return ScheduleType::Monthly;
    }
    return ScheduleType::Daily;
}

bool DateRangeEvent::isInYearRange( const GregorianCalendar::Date& date ) const {
    auto startHash =
        getYearHash( m_yearRange.first, m_monthRange.first, m_dayRange.first, m_hourRange.first, m_minuteRange.first );
    auto endHash = getYearHash(
        m_yearRange.second, m_monthRange.second, m_dayRange.second, m_hourRange.second, m_minuteRange.second );
    auto currentHash = getYearHash( date.getYear(), date.getMonth(), date.getDay(), date.getHour(), date.getMinute() );
    return isInRange( currentHash, startHash, endHash );
}

bool DateRangeEvent::isInMonthRange( const GregorianCalendar::Date& date ) const {
    auto startHash = getMonthHash( m_monthRange.first, m_dayRange.first, m_hourRange.first, m_minuteRange.first );
    auto endHash = getMonthHash( m_monthRange.second, m_dayRange.second, m_hourRange.second, m_minuteRange.second );
    auto currentHash = getMonthHash( date.getMonth(), date.getDay(), date.getHour(), date.getMinute() );
    return isInRange( currentHash, startHash, endHash );
}

bool DateRangeEvent::isInDayRange( const GregorianCalendar::Date& date ) const {
    auto startHash = getDayHash( m_dayRange.first, m_hourRange.first, m_minuteRange.first );
    auto endHash = getDayHash( m_dayRange.second, m_hourRange.second, m_minuteRange.second );
    auto currentHash = getDayHash( date.getDay(), date.getHour(), date.getMinute() );
    return isInRange( currentHash, startHash, endHash );
}

bool DateRangeEvent::isInTimeRange( const GregorianCalendar::Date& date ) const {
    auto startHash = getTimeHash( m_hourRange.first, m_minuteRange.first );
    auto endHash = getTimeHash( m_hourRange.second, m_minuteRange.second );
    auto currentHash = getTimeHash( date.getHour(), date.getMinute() );
    return isInRange( currentHash, startHash, endHash );
}

bool DateRangeEvent::isInRange( uint64_t currentHash, uint64_t startHash, uint64_t endHash ) const {
    if( startHash <= endHash ) {
        return currentHash >= startHash && currentHash < endHash;
    }
    else {
        return currentHash >= startHash || currentHash < endHash;
    }
}

bool DateRangeEvent::isAllDayRange() const {
    return ( m_minuteRange.first == 0 && m_minuteRange.second == 59 && m_hourRange.first == 0 &&
             m_hourRange.second == 23 ) ||
           ( m_minuteRange.first == 59 && m_minuteRange.second == 0 && m_hourRange.first == 23 &&
             m_hourRange.second == 0 );
}

bool DateRangeEvent::isAllMonthRange() const {
    return ( m_dayRange.first == 1 &&
             m_dayRange.second == GregorianCalendar::Date::getDaysInMonth( m_monthRange.second ) ) ||
           ( m_dayRange.first ==
             GregorianCalendar::Date::getDaysInMonth( m_monthRange.second && m_dayRange.second == 1 ) );
}

bool DateRangeEvent::isAllYearRange() const {
    return ( m_monthRange.first == 1 && m_monthRange.second == 12 ) ||
           ( m_monthRange.first == 12 && m_monthRange.second == 1 );
}

bool DateRangeEvent::isForeverRange() const {
    auto maxYear = std::numeric_limits< Year >::max();
    return ( m_yearRange.first == 1 && m_yearRange.second == maxYear ) ||
           ( m_yearRange.first == maxYear && m_yearRange.second == 1 );
}

DateEventBase::OptionalClock DateRangeEvent::getNextDate( const GregorianCalendar::TimePoint& localNow,
                                                          TimeZoneOffset offset,
                                                          const GregorianCalendar::Date& calculatedDate ) const {
    LocalTimePoint tp( localNow.time_since_epoch(), offset, true );
    auto calculatedUtc = calculatedDate.toClockDuration();
    using namespace GregorianCalendar;

    if( calculatedUtc > tp.toUtc() ) {
        auto targetDate = GregorianCalendar::Date::toLocalTime( calculatedDate.toClockDuration(),
                                                                calculatedDate.getTimeZoneOffset() );

        return std::make_unique< LocalTimePoint >( targetDate, calculatedDate.getTimeZoneOffset(), true );
    }
    return nullptr;
}

void DateRangeEvent::setScheduleType() {
    m_scheduleType = getScheduleType();
}

uint64_t DateRangeEvent::getYearHash( Year year, Month month, Day day, Hour hour, Minute minute ) const {
    return (uint64_t) year * 100000000ULL + getMonthHash( month, day, hour, minute );
}

uint64_t DateRangeEvent::getMonthHash( Month month, Day day, Hour hour, Minute minute ) const {
    return (uint64_t) month * 1000000ULL + getDayHash( day, hour, minute );
}

uint64_t DateRangeEvent::getDayHash( Day day, Hour hour, Minute minute ) const {
    return (uint64_t) day * 10000ULL + getTimeHash( hour, minute );
}

uint64_t DateRangeEvent::getTimeHash( DateRangeEvent::Hour hour, DateRangeEvent::Minute minute ) const {
    return (uint64_t) hour * 100ULL + minute;
}

void DateRangeEvent::printHash() const {
    uint64_t startHash = 0;
    uint64_t endHash = 0;
    switch( m_scheduleType ) {
    case ScheduleType::Daily:
        startHash = getTimeHash( m_hourRange.first, m_minuteRange.first );
        endHash = getTimeHash( m_hourRange.second, m_minuteRange.second );
        break;
    case ScheduleType::Monthly:
        startHash = getDayHash( m_dayRange.first, m_hourRange.first, m_minuteRange.first );
        endHash = getDayHash( m_dayRange.second, m_hourRange.second, m_minuteRange.second );
        break;
    case ScheduleType::Yearly:
        startHash = getMonthHash( m_monthRange.first, m_dayRange.first, m_hourRange.first, m_minuteRange.first );
        endHash = getMonthHash( m_monthRange.second, m_dayRange.second, m_hourRange.second, m_minuteRange.second );
        break;
    case ScheduleType::Years:
        startHash = getYearHash(
            m_yearRange.first, m_monthRange.first, m_dayRange.first, m_hourRange.first, m_minuteRange.first );
        endHash = getYearHash(
            m_yearRange.second, m_monthRange.second, m_dayRange.second, m_hourRange.second, m_minuteRange.second );
        break;
    }
    LOGGER_TRACE( "startHash: " + cc::num( startHash ) + ", endHash: " + cc::num( endHash ) );
}

}  // namespace SceneEngine
