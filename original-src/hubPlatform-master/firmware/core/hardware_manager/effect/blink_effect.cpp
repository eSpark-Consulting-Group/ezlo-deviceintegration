#include "hardware_manager/effect/blink_effect.h"
#include <thread>
#include <chrono>

BlinkEffect::BlinkEffect( LedColor& led )
: LedColorEffect( led )
, m_color( 0, 0, 0 )
, m_count( 0 )
, m_delay( 0 )
, m_gradual( 0 ) {}

void BlinkEffect::effect() {
    blinkingRgbColor( m_color, m_count, m_delay, m_gradual );
    m_led.turnOff();
}

void BlinkEffect::setBlinkParameters( Rgb color, int count, int delay, int gradual ) {
    m_color = color;
    m_count = count;
    m_delay = delay;
    m_gradual = gradual;
}
