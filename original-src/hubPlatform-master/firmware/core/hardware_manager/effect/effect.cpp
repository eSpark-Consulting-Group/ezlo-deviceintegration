#include "hardware_manager/effect/effect.h"
#include <string>

Effect::Effect()
: m_shouldBeStopped( false )
, m_stopRequestHandled( false ) {}

void Effect::start() {
    if( !m_shouldBeStopped || m_stopRequestHandled ) {
        effect();
    }
    m_stopRequestHandled = true;
    m_shouldBeStopped = false;
}

void Effect::stop() {
    m_shouldBeStopped = true;
}

bool Effect::isRunning() const {
    return !m_shouldBeStopped;
}

BlinkingType Effect::blinkingTypeFromString( const std::string& type ) {
    if( type == "on" ) {
        return BlinkingType::On;
    }
    else if( type == "blink" ) {
        return BlinkingType::Blink;
    }
    else if( type == "breathe" ) {
        return BlinkingType::Breathe;
    }
    else {
        return BlinkingType::None;
    }
}

Color Effect::colorFromString( const std::string& color ) {
    if( color == "red" ) {
        return Color::Red;
    }
    else if( color == "green" ) {
        return Color::Green;
    }
    else if( color == "blue" ) {
        return Color::Blue;
    }
    else if( color == "yellow" ) {
        return Color::Yellow;
    }
    else if( color == "orange" ) {
        return Color::Orange;
    }
    else if( color == "magenta" ) {
        return Color::Magenta;
    }
    else if( color == "cyan" ) {
        return Color::Cyan;
    }
    else if( color == "white" ) {
        return Color::White;
    }

    else {
        return Color::None;
    }
}