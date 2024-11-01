#pragma once
// Extracted from date_event.h/cpp
#include "scenes_manager/scenes_engine/events/date_event.h"

namespace SceneEngine {

class OnceEvent : public DateEvent {
public:
    explicit OnceEvent( const GregorianCalendar::Date& date );

    std::string getMethodName() const override;
    OptionalClock getNextDate( const GregorianCalendar::TimePoint& localNow, TimeZoneOffset offset ) const override;

private:
    GregorianCalendar::Date m_date;
};

}  // namespace SceneEngine
