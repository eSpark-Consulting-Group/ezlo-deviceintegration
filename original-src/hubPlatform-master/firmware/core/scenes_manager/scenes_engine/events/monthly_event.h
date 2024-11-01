#pragma once
// Extracted from date_event.h/cpp
#include "scenes_manager/scenes_engine/events/date_event.h"

namespace SceneEngine {

class MonthlyEvent : public DateEvent {
public:
    MonthlyEvent( TimeList timeList, std::set< Day > days );
    OptionalClock getNextDate( const GregorianCalendar::TimePoint& localNow, TimeZoneOffset offset ) const override;

private:
    TimeList m_timeList;
    std::set< Day > m_dayList;
};

}  // namespace SceneEngine