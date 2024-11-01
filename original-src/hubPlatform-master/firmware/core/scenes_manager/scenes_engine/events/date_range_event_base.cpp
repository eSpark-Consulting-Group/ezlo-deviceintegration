// Extracted from event_base.h/cpp
#include "scenes_manager/scenes_engine/events/date_range_event_base.h"
#include "log_utils/ezlo_log.h"

namespace SceneEngine {

DateRangeEventBase::DateRangeEventBase()
: m_isInRange( false ) {}

/** @returns value stored by the previous call to update() */
bool DateRangeEventBase::isInRange() const {
    return m_isInRange;
}

/** Sets m_inRange member field to the value returned by isTimePointRange method.
 * @returns true if the value differs from the previous one. */
bool DateRangeEventBase::update( const GregorianCalendar::TimePoint& localNow, TimeZoneOffset offset, bool verbose ) {
    const bool nowInRange = isTimePointInRange( localNow, offset, verbose );
    const bool wasInRange = m_isInRange.exchange( nowInRange, std::memory_order_relaxed );
    return wasInRange != nowInRange;
}

}  // namespace SceneEngine
