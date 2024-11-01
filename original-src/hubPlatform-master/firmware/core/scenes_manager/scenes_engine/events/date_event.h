#pragma once

#include "scenes_manager/scenes_engine/events/date_event_base.h"
#include "base_utils/gregorian_calendar.h"
#include <set>

namespace SceneEngine {

class DateEvent : public DateEventBase {
public:
    using TimeType = std::pair< uint8_t, uint8_t >;
    using TimeList = std::set< TimeType >;

    std::string getMethodName() const override;

protected:
    OptionalClock findNextTime( GregorianCalendar::ClockDuration now,
                                GregorianCalendar::Date& date,
                                const TimeList& timeList ) const;
};

using DateEventPtr = std::shared_ptr< DateEventBase >;

}  // namespace SceneEngine
