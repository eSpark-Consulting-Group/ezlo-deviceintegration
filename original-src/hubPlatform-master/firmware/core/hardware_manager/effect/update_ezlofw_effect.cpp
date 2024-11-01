#include "hardware_manager/effect/update_ezlofw_effect.h"
#include "hardware_manager/utils/rgb.h"

#include <thread>
#include <chrono>

void updateEzloFWEffectPreset( LedColor& led ) {
    led.setColor( Rgb::Magenta );
    led.turnBlinking( 1000, 1000 );
}

UpdateEzloFWEffect::UpdateEzloFWEffect( LedColor& led )
: LedColorEffect( led ) {}

void UpdateEzloFWEffect::effect() {
    updateEzloFWEffectPreset( m_led );
    while( isRunning() ) {
        std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
    }
}
