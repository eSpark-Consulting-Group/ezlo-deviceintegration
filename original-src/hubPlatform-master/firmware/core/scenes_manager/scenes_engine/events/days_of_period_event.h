#pragma once

#include "scenes_manager/scenes_engine/events/date_range_event_base.h"

namespace SceneEngine {

class DaysOfPeriodEvent : public DateRangeEventBase {
public:
    enum class Period { Month, Week };

    DaysOfPeriodEvent( Period period, std::set< uint8_t > days );

    std::string getMethodName() const override;
    OptionalClock getNextDate( const GregorianCalendar::TimePoint& localNow, TimeZoneOffset offset ) const override;

private:
    bool isTimePointInRange( const GregorianCalendar::TimePoint& localNow,
                             TimeZoneOffset offset,
                             bool verbose ) const override;

    uint8_t getDayOfPeriod( const GregorianCalendar::Date& date ) const;
    void verifyDaysValues() const;
    OptionalClock findNextDate( const GregorianCalendar::TimePoint& localNow,
                                TimeZoneOffset offset,
                                const GregorianCalendar::Date& calculatedDate ) const;

private:
    std::set< uint8_t > m_daysList;
    Period m_period;
};

}  // namespace SceneEngine
