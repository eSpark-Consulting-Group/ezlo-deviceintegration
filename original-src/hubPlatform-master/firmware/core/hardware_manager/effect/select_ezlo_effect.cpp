#include "hardware_manager/effect/select_ezlo_effect.h"
#include "hardware_manager/utils/rgb.h"

SelectEzloEffect::SelectEzloEffect( LedColor& led )
: LedColorEffect( led ) {}

void SelectEzloEffect::effect() {
    blinkingRgbColor( Rgb::Green, 20, 1, 10 );
    gradualRgbColor( Rgb::Green, 10, 2000 );
    m_led.turnOff();
}
