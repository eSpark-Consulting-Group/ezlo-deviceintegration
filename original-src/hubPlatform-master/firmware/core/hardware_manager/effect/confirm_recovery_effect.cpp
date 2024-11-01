#include "hardware_manager/effect/confirm_recovery_effect.h"
#include "hardware_manager/utils/rgb.h"

ConfirmRecoveryEffect::ConfirmRecoveryEffect( LedColor& led )
: LedColorEffect( led ) {}

void ConfirmRecoveryEffect::effect() {
    while( isRunning() ) {
        blinkingRgbColor( Rgb::LightOrange, 1000, 300, 50 );
    }
    m_led.turnOff();
}
