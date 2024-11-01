#include "hardware_manager/effect/ok_color_effect.h"
#include "hardware_manager/utils/rgb.h"

OkColorEffect::OkColorEffect( LedColor& led )
: LedColorEffect( led ) {}

void OkColorEffect::effect() {
    gradualRgbColor( Rgb::Green, 100, 2000 );
    m_led.turnOff();
}
