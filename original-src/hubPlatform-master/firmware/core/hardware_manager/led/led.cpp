#include "hardware_manager/led/led.h"

Led::Led( std::string name )
: m_name( std::move( name ) )
, m_locked( false )
, state( LedState::LedUnknown ) {}

LedState Led::getState() const {
    return state;
}

bool Led::isLocked() const {
    return m_locked;
}

void Led::lock() {
    m_locked = true;
}

void Led::unlock() {
    m_locked = false;
}

const std::string& Led::getName() const {
    return m_name;
}