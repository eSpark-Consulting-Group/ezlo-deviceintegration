#pragma once

#include "hardware_manager/button/button_state.h"
#include "base_utils/gpio.h"
#include <chrono>

namespace HubPlatform {
namespace HardwareManager {

class GpioButtonState : public ButtonState {
public:
    GpioButtonState( int gpioNumber );

    void process() override;
    std::chrono::milliseconds holdDuration() const override;
    std::chrono::milliseconds pulseDuration() const override;

private:
    common::Gpio m_button;
    common::GpioState m_previousState;
    common::GpioState m_currentState;
    std::chrono::time_point< std::chrono::steady_clock > m_pulseTime;
    std::chrono::milliseconds m_resultPulseTime;
};

}  // namespace HardwareManager
}  // namespace HubPlatform
