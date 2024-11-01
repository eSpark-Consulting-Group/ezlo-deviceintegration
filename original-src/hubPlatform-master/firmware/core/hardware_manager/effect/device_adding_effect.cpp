#include "hardware_manager/effect/device_adding_effect.h"

DeviceAddingEffect::DeviceAddingEffect( LedColor& led )
: LedColorEffect( led ) {}

void DeviceAddingEffect::effect() {
    wavingRgbColor( Rgb( 0xFF, 0x00, 0xFF ), Rgb::Blue, -1, 160, 32 );
}
