#pragma once

#include "utils/timer/timer.h"

namespace TwoGig {

class TimerImpl : public Timer {
public:
    TimePoint timestamp() const override;
};

}  // namespace TwoGig
