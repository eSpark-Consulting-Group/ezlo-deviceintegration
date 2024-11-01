#pragma once

#include "utils/timer/timer.h"

namespace TwoGig {
namespace Test {

class FakeTimer : public Timer {
public:
    TimePoint timestamp() const override {
        return m_timepoint;
    }

    void advanceTime( Duration duration ) {
        m_timepoint += duration;
    }

private:
    TimePoint m_timepoint;
};

}  // namespace Test
}  // namespace TwoGig
