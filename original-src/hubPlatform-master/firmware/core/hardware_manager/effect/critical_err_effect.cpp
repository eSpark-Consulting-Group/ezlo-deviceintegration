#include "hardware_manager/effect/critical_err_effect.h"
#include "hardware_manager/utils/rgb.h"

CriticalErrEffect::CriticalErrEffect( LedColor& led )
: LedColorEffect( led )
, m_counter( 0 ) {}

void CriticalErrEffect::effect() {
    m_counter = 0;
    while( isRunning() && m_counter < 5 ) {
        gradualRgbColor( Rgb::Red, 30, 2000 );
        blinkingRgbColor( Rgb::Red, 5, 1, 30 );
        m_counter++;
    }
}
