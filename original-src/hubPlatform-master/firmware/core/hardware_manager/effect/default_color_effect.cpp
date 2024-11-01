#include "hardware_manager/effect/default_color_effect.h"
#include "hardware_manager/utils/rgb.h"

#include <thread>
#include <chrono>

DefaultColorEffect::DefaultColorEffect( LedColor& led )
: LedColorEffect( led ) {}

void DefaultColorEffect::effect() {
    gradualRgbColor( Rgb::LightBlue );
    while( isRunning() ) {
        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
    }
}
