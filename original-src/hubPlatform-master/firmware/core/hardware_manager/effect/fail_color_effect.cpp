#include "hardware_manager/effect/fail_color_effect.h"
#include "hardware_manager/utils/rgb.h"

FailColorEffect::FailColorEffect( LedColor& led )
: LedColorEffect( led ) {}

void FailColorEffect::effect() {
    gradualRgbColor( Rgb::Red, 100, 2000 );
    m_led.turnOff();
}
