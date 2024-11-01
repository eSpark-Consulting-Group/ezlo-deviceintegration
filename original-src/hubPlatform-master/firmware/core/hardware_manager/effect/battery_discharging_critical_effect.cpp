#include "hardware_manager/effect/battery_discharging_critical_effect.h"

BatteryDischargingCriticalEffect::BatteryDischargingCriticalEffect( LedColor& led )
: LedColorEffect( led ) {}

void BatteryDischargingCriticalEffect::effect() {
    wavingRgbColor( Rgb::LightBlue, Rgb::Red, -1, 1000, 132 );
}
