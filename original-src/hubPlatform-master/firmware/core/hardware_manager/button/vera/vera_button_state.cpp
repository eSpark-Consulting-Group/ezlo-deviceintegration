#include "hardware_manager/button/vera/vera_button_state.h"
#include "hardware_manager/button/vera/vera_buttons_handler.h"
#include <functional>

namespace HubPlatform {
namespace HardwareManager {

VeraButtonState::VeraButtonState( std::string buttonName )
: m_buttonName( std::move( buttonName ) )
, m_previousState( ButtonState::StateValue::Released )
, m_currentState( ButtonState::StateValue::Released )
, m_resultPulseTime( emptyDuration() ) {
    VeraButtonsHandler::instance().addButtonStateHandler(
        m_buttonName,
        std::bind( &VeraButtonState::onStateChanged, this, std::placeholders::_1, std::placeholders::_2 ) );
}

VeraButtonState::~VeraButtonState() {
    VeraButtonsHandler::instance().removeButtonStateHandler( m_buttonName );
}

void VeraButtonState::process() {
    using BS = ButtonState::StateValue;
    m_resultPulseTime = emptyDuration();
    if( m_currentState == BS::Pressed && m_previousState == BS::Released ) {
        m_pulseTime = std::chrono::steady_clock::now();
    }
    else if( m_currentState == BS::Released && m_previousState == BS::Pressed ) {
        auto now = std::chrono::steady_clock::now();
        m_resultPulseTime = std::chrono::duration_cast< std::chrono::milliseconds >( now - m_pulseTime );
    }
    m_previousState = m_currentState;
}

std::chrono::milliseconds VeraButtonState::holdDuration() const {
    if( m_currentState != ButtonState::StateValue::Pressed )
        return emptyDuration();

    auto now = std::chrono::steady_clock::now();
    auto holdTime = std::chrono::duration_cast< std::chrono::milliseconds >( now - m_pulseTime );
    return holdTime;
}

std::chrono::milliseconds VeraButtonState::pulseDuration() const {
    return m_resultPulseTime;
}

void VeraButtonState::onStateChanged( ButtonState::StateValue state, unsigned int /*secondsPressed*/ ) {
    m_currentState = state;
}

}  // namespace HardwareManager
}  // namespace HubPlatform
