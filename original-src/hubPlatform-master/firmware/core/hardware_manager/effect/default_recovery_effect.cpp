#include "hardware_manager/effect/default_recovery_effect.h"
#include "hardware_manager/utils/rgb.h"

DefaultRecoveryEffect::DefaultRecoveryEffect( LedColor& led )
: LedColorEffect( led )
, m_counter( 0 ) {}

void DefaultRecoveryEffect::effect() {
    m_counter = 0;
    while( isRunning() && m_counter < 100 ) {
        gradualRgbColor( Rgb::LightOrange );
        gradualRgbColor( Rgb::Yellow, 300, 3000 );
        gradualRgbColor( Rgb::LightOrange, 300, 30000 );
        m_counter++;
    }
    m_led.turnOff();
}
