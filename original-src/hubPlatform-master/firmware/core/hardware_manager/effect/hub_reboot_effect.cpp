#include "hardware_manager/effect/hub_reboot_effect.h"
#include "hardware_manager/utils/rgb.h"

#include <thread>
#include <chrono>

void hubRebootEffectPreset( LedColor& led ) {
    led.setColor( Rgb::Cyan );
    led.turnBlinking( 125, 125 );
}

HubRebootEffect::HubRebootEffect( LedColor& led )
: LedColorEffect( led ) {}

void HubRebootEffect::effect() {
    hubRebootEffectPreset( m_led );
    while( isRunning() ) {
        std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
    }
    m_led.turnOff();
}
