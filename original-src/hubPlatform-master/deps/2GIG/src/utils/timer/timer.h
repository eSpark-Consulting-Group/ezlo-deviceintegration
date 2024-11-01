#pragma once

#include <chrono>

namespace TwoGig {

struct Timer {
    using Clock = std::chrono::system_clock;
    using TimePoint = Clock::time_point;
    using Duration = Clock::duration;

public:
    virtual ~Timer() = default;

    virtual TimePoint timestamp() const = 0;
};

}  // namespace TwoGig
