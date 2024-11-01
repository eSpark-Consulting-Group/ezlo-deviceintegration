#pragma once
// Extracted from event_base.h/cpp
#include "scenes_manager/scenes_engine/events/event_base.h"
#include "base_utils/gregorian_calendar.h"
#include "scenes_manager/scenes_engine/date_scheduler/local_timepoint.h"

namespace SceneEngine {

class DateEventBase : public EventBase {
public:
    using Minute = uint8_t;
    using Hour = uint8_t;
    using Day = uint8_t;
    using Month = uint8_t;
    using Week = int8_t;
    using Year = uint16_t;

    using OptionalClock = std::unique_ptr< LocalTimePoint >;

    /** Calculates time point of the next date-based event.
     * @returns timestamp of the next event, local time;
     * @returns null - no next event can be calculated */
    virtual OptionalClock getNextDate( const GregorianCalendar::TimePoint& localNow,
                                       TimeZoneOffset timeZoneOffset ) const = 0;
};

using DateEventBasePtr = std::shared_ptr< DateEventBase >;

}  // namespace SceneEngine
