#include "utils/timer/timer_impl.h"

namespace TwoGig {

Timer::TimePoint TimerImpl::timestamp() const {
    return Clock::now();
}

}  // namespace TwoGig
