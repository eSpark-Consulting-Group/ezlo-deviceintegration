#pragma once

#include <chrono>

namespace SceneEngine {

class IClock {
public:
    virtual ~IClock() = default;

    virtual std::chrono::system_clock::time_point greenwichMeanTimePoint() = 0;
    virtual std::chrono::system_clock::duration greenwichMeanTimeSinceEpoch() = 0;

    virtual std::chrono::system_clock::time_point localTime() = 0;
    virtual std::chrono::system_clock::duration localTimeSinceEpoch() = 0;

    virtual std::chrono::seconds timeZoneOffset() = 0;
    /** Calculates time zone offset for a specific time point. Resets timeInfo.tm_isdst to -1 to calculate actual Day
     * Savings Time value.
     * @param timeInfo [in/out] time information to normalize
     * @return number fo seconds for time zone offset for a time point specified
     */
    virtual std::chrono::seconds timeZoneOffset( std::chrono::system_clock::time_point ) const = 0;
};

}  // namespace SceneEngine
