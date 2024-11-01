#include "hardware_manager/effect/hub_power_off_effect.h"
#include "hardware_manager/utils/rgb.h"

HubPowerOffEffect::HubPowerOffEffect( LedColor& led )
: LedColorEffect( led ) {}

void HubPowerOffEffect::effect() {
    while( isRunning() ) {
        blinkingRgbColor( Rgb::Yellow, 100, 40 );
    }

    m_led.turnOff();
}
