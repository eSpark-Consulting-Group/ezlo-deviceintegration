#include "hardware_manager/effect/hub_not_connected_effect.h"

HubNotConnectedEffect::HubNotConnectedEffect( LedColor& led )
: LedColorEffect( led ) {}

void HubNotConnectedEffect::effect() {
    blinkingRgbColor( Rgb::LightOrange, -1, 80, 16 );
}
