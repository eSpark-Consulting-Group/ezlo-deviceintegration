#include "hardware_manager/effect/lock_ezlo_effect.h"
#include "hardware_manager/utils/rgb.h"

LockEzloEffect::LockEzloEffect( LedColor& led )
: LedColorEffect( led ) {}

void LockEzloEffect::effect() {
    while( isRunning() ) {
        gradualRgbColor( Rgb::White, 500, 5000 );
        gradualRgbColor( Rgb::DarkGray, 500, 200 );
    }
    m_led.turnOff();
}
