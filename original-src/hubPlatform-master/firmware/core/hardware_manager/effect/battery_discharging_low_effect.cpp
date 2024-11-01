#include "hardware_manager/effect/battery_discharging_low_effect.h"

BatteryDischargingLowEffect::BatteryDischargingLowEffect( LedColor& led )
: LedColorEffect( led ) {}

void BatteryDischargingLowEffect::effect() {
    wavingRgbColor( Rgb::LightBlue, Rgb::Yellow, -1, 1000, 132 );
}
