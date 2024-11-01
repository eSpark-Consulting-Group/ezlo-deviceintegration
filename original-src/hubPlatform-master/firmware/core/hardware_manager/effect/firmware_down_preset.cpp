#include "hardware_manager/effect/firmware_down_preset.h"

void firmwareDownPreset( LedColor& led ) {
    led.setColor( Rgb::Yellow );
    led.turnOn();
}
