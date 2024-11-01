#include "hardware_manager/effect/device_removing_effect.h"

DeviceRemovingEffect::DeviceRemovingEffect( LedColor& led )
: LedColorEffect( led ) {}

void DeviceRemovingEffect::effect() {
    wavingRgbColor( Rgb( 0xFF, 0x00, 0xFF ), Rgb::Blue, -1, 80, 16 );
}
