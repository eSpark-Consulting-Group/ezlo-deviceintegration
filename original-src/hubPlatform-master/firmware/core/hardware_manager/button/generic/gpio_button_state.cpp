#include "hardware_manager/button/generic/gpio_button_state.h"

namespace HubPlatform {
namespace HardwareManager {

GpioButtonState::GpioButtonState( int gpioNumber )
: m_button( gpioNumber )
, m_previousState( common::GpioState::ON )
, m_currentState( common::GpioState::ON )
, m_resultPulseTime( emptyDuration() ) {
    m_button.setDirection( common::GpioDirection::IN );
    m_button.setEdgeDetecting( common::GpioEdge::BOTH );
}

void GpioButtonState::process() {
    m_resultPulseTime = emptyDuration();
    m_currentState = m_button.getState();
    if( m_currentState == common::GpioState::OFF && m_previousState == common::GpioState::ON ) {
        m_pulseTime = std::chrono::steady_clock::now();
    }
    else if( m_currentState == common::GpioState::ON && m_previousState == common::GpioState::OFF ) {
        auto now = std::chrono::steady_clock::now();
        m_resultPulseTime = std::chrono::duration_cast< std::chrono::milliseconds >( now - m_pulseTime );
    }
    m_previousState = m_currentState;
}

std::chrono::milliseconds GpioButtonState::holdDuration() const {
    if( m_currentState == common::GpioState::ON )
        return emptyDuration();

    auto now = std::chrono::steady_clock::now();
    auto holdTime = std::chrono::duration_cast< std::chrono::milliseconds >( now - m_pulseTime );
    return holdTime;
}

std::chrono::milliseconds GpioButtonState::pulseDuration() const {
    return m_resultPulseTime;
}

}  // namespace HardwareManager
}  // namespace HubPlatform
