#include "hardware_manager/effect/battery_discharging_effect.h"

BatteryDischargingEffect::BatteryDischargingEffect( LedColor& led )
: LedColorEffect( led ) {}

void BatteryDischargingEffect::effect() {
    wavingRgbColor( Rgb::LightBlue, Rgb::Black, -1, 1000, 132 );
}
