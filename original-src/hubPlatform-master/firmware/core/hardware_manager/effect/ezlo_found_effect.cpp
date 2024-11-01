#include "hardware_manager/effect/ezlo_found_effect.h"
#include "hardware_manager/utils/rgb.h"

EzloFoundEffect::EzloFoundEffect( LedColor& led )
: LedColorEffect( led ) {}

void EzloFoundEffect::effect() {
    blinkingRgbColor( Rgb::Pink, 5, 1, 10 );
    m_led.turnOff();
}
