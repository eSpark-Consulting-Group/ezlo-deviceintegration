#include "hardware_manager/effect/hub_factory_reset_effect.h"
#include "hardware_manager/utils/rgb.h"

#include <thread>
#include <chrono>

void hubFactoryResetEffectPreset( LedColor& led ) {
    led.setColor( Rgb::White );
    led.turnBlinking( 1000, 1000 );
}

HubFactoryResetEffect::HubFactoryResetEffect( LedColor& led )
: LedColorEffect( led ) {}

void HubFactoryResetEffect::effect() {
    hubFactoryResetEffectPreset( m_led );
    while( isRunning() ) {
        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
    }
}
