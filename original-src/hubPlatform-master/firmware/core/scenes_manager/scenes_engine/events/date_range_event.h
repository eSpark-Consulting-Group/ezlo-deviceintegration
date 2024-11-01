#pragma once

#include "scenes_manager/scenes_engine/events/date_range_event_base.h"
#include "scenes_manager/scenes_engine/events/event_base.h"
#include "base_utils/gregorian_calendar.h"
#include <set>
#include <chrono>
#include <atomic>

namespace SceneEngine {

namespace UT {
class DataRangeEventTests;
}

class DateRangeEvent : public DateRangeEventBase {
public:
    using YearRange = std::pair< Year, Year >;
    using MonthRange = std::pair< Month, Month >;
    using DayRange = std::pair< Day, Day >;
    using HourRange = std::pair< Hour, Hour >;
    using MinuteRange = std::pair< Minute, Minute >;

    enum class ScheduleType : int { Daily, Monthly, Yearly, Years };

public:
    DateRangeEvent();

    bool setYearRange( Year begin, Year end );
    bool setMonthAndDayRanges( const MonthRange& monthRange, const DayRange& dayRange );
    bool setHourRange( Hour begin, Hour end );
    bool setMinuteRange( Minute begin, Minute end );

    std::string getMethodName() const override;
    OptionalClock getNextDate( const GregorianCalendar::TimePoint& localNow, TimeZoneOffset offset ) const override;
    bool isTimePointInRange( const GregorianCalendar::TimePoint& localNow,
                             TimeZoneOffset offset,
                             bool verbose ) const override;

    ScheduleType getScheduleType() const;

private:
    bool isInYearRange( const GregorianCalendar::Date& date ) const;
    bool isInMonthRange( const GregorianCalendar::Date& date ) const;
    bool isInDayRange( const GregorianCalendar::Date& date ) const;
    bool isInTimeRange( const GregorianCalendar::Date& date ) const;
    bool isInRange( uint64_t currentHash, uint64_t startHash, uint64_t endHash ) const;

    bool isAllDayRange() const;
    bool isAllMonthRange() const;
    bool isAllYearRange() const;
    bool isForeverRange() const;

    OptionalClock getNextDate( const GregorianCalendar::TimePoint& localNow,
                               TimeZoneOffset offset,
                               const GregorianCalendar::Date& calculatedDate ) const;

    void setScheduleType();

    uint64_t getYearHash( Year year, Month month, Day day, Hour hour, Minute minute ) const;
    uint64_t getMonthHash( Month month, Day day, Hour hour, Minute minute ) const;
    uint64_t getDayHash( Day day, Hour hour, Minute minute ) const;
    uint64_t getTimeHash( Hour hour, Minute minute ) const;

    void printHash() const;

private:
    YearRange m_yearRange;
    MonthRange m_monthRange;
    DayRange m_dayRange;
    HourRange m_hourRange;
    MinuteRange m_minuteRange;
    ScheduleType m_scheduleType;

    friend class UT::DataRangeEventTests;
};

using DateRangeEventPtr = std::shared_ptr< DateRangeEvent >;

}  // namespace SceneEngine
