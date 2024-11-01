#pragma once
// Extracted from date_event.h/cpp
#include "scenes_manager/scenes_engine/events/date_event.h"

namespace SceneEngine {

class DailyEvent : public DateEvent {
public:
    explicit DailyEvent( TimeList timeList );
    OptionalClock getNextDate( const GregorianCalendar::TimePoint& localNow, TimeZoneOffset offset ) const override;

private:
    TimeList m_timeList;
};

}  // namespace SceneEngine