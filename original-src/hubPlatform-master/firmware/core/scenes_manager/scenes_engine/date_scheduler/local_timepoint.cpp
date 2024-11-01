#include "scenes_manager/scenes_engine/date_scheduler/local_timepoint.h"

namespace SceneEngine {

LocalTimePoint::LocalTimePoint( GregorianCalendar::ClockDuration duration, TimeZoneOffset offset, bool isLocalTime )
: m_duration( duration )
, m_offset( offset )
, m_isLocalTime( isLocalTime ) {}

void LocalTimePoint::setLocalTime( GregorianCalendar::ClockDuration duration,
                                   TimeZoneOffset tzOffset,
                                   bool isLocalTime ) {
    m_duration = duration;
    m_offset = tzOffset;
    m_isLocalTime = isLocalTime;
}

GregorianCalendar::ClockDuration LocalTimePoint::duration() const {
    return m_duration;
}

TimeZoneOffset LocalTimePoint::offset() const {
    return m_offset;
}

bool LocalTimePoint::isLocalTime() const {
    return m_isLocalTime;
}

GregorianCalendar::ClockDuration LocalTimePoint::toUtc() const {
    if( m_isLocalTime )
        return duration() - offset();
    return duration();
}

}  // namespace SceneEngine