#pragma once

#include "scenes_manager/scenes_engine/events/date_range_event_base.h"

namespace SceneEngine {

class WeeksOfPeriodEvent : public DateRangeEventBase {
public:
    enum class Period { Month, Year };

    WeeksOfPeriodEvent( Period period, std::set< Week > weeks );

    std::string getMethodName() const override;
    OptionalClock getNextDate( const GregorianCalendar::TimePoint& localNow, TimeZoneOffset offset ) const override;

private:
    bool isTimePointInRange( const GregorianCalendar::TimePoint& localNow,
                             TimeZoneOffset offset,
                             bool verbose ) const override;

    uint8_t getWeekOfPeriod( const GregorianCalendar::Date& date ) const;

    bool isLastWeekOfPeriod( const GregorianCalendar::Date& date ) const;
    static bool isLastWeekOfMonth( GregorianCalendar::Date date );
    static bool isLastWeekOfYear( GregorianCalendar::Date date );
    OptionalClock findNextDate( const GregorianCalendar::TimePoint& localNow,
                                TimeZoneOffset offset,
                                const GregorianCalendar::Date& calculatedDate ) const;

private:
    std::set< Week > m_weekList;
    Period m_period;
};

}  // namespace SceneEngine
