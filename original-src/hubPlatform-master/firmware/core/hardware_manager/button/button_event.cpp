#include "hardware_manager/button/button_event.h"

ButtonEvent::ButtonEvent( int priority, ButtonEventType type, std::string name, int secondsPassed )
: Event( priority )
, m_type( type )
, m_name( std::move( name ) )
, m_secondsPassed( secondsPassed ) {}

ButtonEvent::ButtonEvent( int priority )
: Event( priority )
, m_type( ButtonEventType::Unknown )
, m_name( "" )
, m_secondsPassed( 0 ) {}

ButtonEventType ButtonEvent::getType() const {
    return m_type;
}

const std::string& ButtonEvent::getName() const {
    return m_name;
}

int ButtonEvent::getSecondsPassed() const {
    return m_secondsPassed;
}

void ButtonEvent::setType( ButtonEventType type ) {
    m_type = type;
}
void ButtonEvent::setName( const std::string& name ) {
    m_name = name;
}
void ButtonEvent::setSecondsPassed( int secondsPassed ) {
    if( secondsPassed >= 0 ) {
        m_secondsPassed = secondsPassed;
    }
}

std::string ButtonEvent::eventTypeString( ButtonEventType eventType ) {
    switch( eventType ) {
    case ButtonEventType::Pressed:
        return "pressed";
    case ButtonEventType::Released:
        return "released";
    default:
    case ButtonEventType::Unknown:
        return "unknown";
    }
}