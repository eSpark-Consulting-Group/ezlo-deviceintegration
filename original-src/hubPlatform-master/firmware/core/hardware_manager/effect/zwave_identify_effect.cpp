#include "hardware_manager/effect/zwave_identify_effect.h"
#include "hardware_manager/utils/rgb.h"

#include <thread>
#include <chrono>

ZwaveIdentifyEffect::ZwaveIdentifyEffect( LedColor& led, Led& zwaveLed )
: LedColorEffect( led )
, m_zwaveLed( zwaveLed ) {}

void ZwaveIdentifyEffect::effect() {
    m_led.setColor( Rgb::Green );

    for( int i = 0; i < 3; ++i ) {
        m_led.turnOff();
        m_zwaveLed.turnOff();
        std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
        m_led.turnOn();
        m_zwaveLed.turnOn();
        std::this_thread::sleep_for( std::chrono::milliseconds( 600 ) );
    }
    m_led.setColor( Rgb::Black );
    m_led.turnOff();
    m_zwaveLed.turnOff();
    std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
}
