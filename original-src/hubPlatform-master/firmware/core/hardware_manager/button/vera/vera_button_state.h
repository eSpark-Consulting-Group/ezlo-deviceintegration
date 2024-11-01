#pragma once
#include "hardware_manager/button/button_state.h"
#include <string>

namespace HubPlatform {
namespace HardwareManager {

class VeraButtonState : public ButtonState {
public:
    VeraButtonState( std::string buttonName );
    ~VeraButtonState() override;

    void process() override;
    std::chrono::milliseconds holdDuration() const override;
    std::chrono::milliseconds pulseDuration() const override;

private:
    void onStateChanged( ButtonState::StateValue state, unsigned int secondsPressed );

private:
    std::string m_buttonName;

    ButtonState::StateValue m_previousState;
    ButtonState::StateValue m_currentState;
    std::chrono::time_point< std::chrono::steady_clock > m_pulseTime;
    std::chrono::milliseconds m_resultPulseTime;
};

}  // namespace HardwareManager
}  // namespace HubPlatform
