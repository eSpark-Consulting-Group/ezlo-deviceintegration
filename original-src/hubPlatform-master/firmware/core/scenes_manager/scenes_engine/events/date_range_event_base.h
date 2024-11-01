#pragma once
// Extracted from event_base.h/cpp
#include "scenes_manager/scenes_engine/events/date_event_base.h"
#include <atomic>

namespace SceneEngine {

/** Base class for date range events: events that should hold their state during the period of time */
class DateRangeEventBase : public DateEventBase {
public:
    DateRangeEventBase();

    bool isInRange() const;
    /** @returns "true" - if crossed an edge of the range */
    bool update( const GregorianCalendar::TimePoint& localNow, TimeZoneOffset offset, bool verbose );

private:
    virtual bool isTimePointInRange( const GregorianCalendar::TimePoint& localNow,
                                     TimeZoneOffset offset,
                                     bool verbose ) const = 0;

    std::atomic_bool m_isInRange;
};

using DateRangeEventBasePtr = std::shared_ptr< DateRangeEventBase >;

}  // namespace SceneEngine
