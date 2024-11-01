#include "hardware_manager/effect/hub_soft_reset_effect.h"
#include "hardware_manager/utils/rgb.h"

#include <thread>
#include <chrono>

void hubSoftResetEffectPreset( LedColor& led ) {
    led.setColor( Rgb::White );
    led.turnBlinking( 300, 300 );
}

HubSoftResetEffect::HubSoftResetEffect( LedColor& led )
: LedColorEffect( led ) {}

void HubSoftResetEffect::effect() {
    hubSoftResetEffectPreset( m_led );
    while( isRunning() ) {
        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
    }
}
