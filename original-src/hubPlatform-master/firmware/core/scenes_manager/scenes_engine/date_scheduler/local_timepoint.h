#pragma once

#include "base_utils/gregorian_calendar.h"
#include <chrono>

namespace SceneEngine {

using TimeZoneOffset = std::chrono::seconds;
struct LocalTimePoint {
public:
    LocalTimePoint() = default;
    explicit LocalTimePoint( GregorianCalendar::ClockDuration duration,
                             TimeZoneOffset offset,
                             bool isLocalTime = true );

    void setLocalTime( GregorianCalendar::ClockDuration localNow, TimeZoneOffset tzOffset, bool isLocalTime = true );

    GregorianCalendar::ClockDuration duration() const;
    TimeZoneOffset offset() const;
    bool isLocalTime() const;
    GregorianCalendar::ClockDuration toUtc() const;

private:
    GregorianCalendar::ClockDuration m_duration{ 0 };
    TimeZoneOffset m_offset{ 0 };
    bool m_isLocalTime{ true };  //!< If true, object contains local time as m_duration. Otherwise, contains UTC.
};

}  // namespace SceneEngine