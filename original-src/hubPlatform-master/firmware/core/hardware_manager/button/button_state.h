#pragma once
#include <chrono>

class ButtonState {
public:
    enum class StateValue { Unknown = 0, Pressed, Released };

    virtual ~ButtonState() = default;

    virtual void process() = 0;

    static std::chrono::milliseconds emptyDuration() {
        return std::chrono::milliseconds::max();
    }

    virtual std::chrono::milliseconds holdDuration() const = 0;
    virtual std::chrono::milliseconds pulseDuration() const = 0;
};
