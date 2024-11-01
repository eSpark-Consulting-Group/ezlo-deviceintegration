#include "hardware_manager/effect/wifi_wps_color_effect.h"
#include "hardware_manager/utils/rgb.h"

WifiWpsColorEffect::WifiWpsColorEffect( LedColor& led, Led& wifiLed )
: LedColorEffect( led )
, m_wifiLed( wifiLed ) {}

void WifiWpsColorEffect::effect() {
    while( isRunning() ) {
        gradualRgbColor( Rgb( 100, 100, 255 ), 50 );
        m_wifiLed.turnOn();
        gradualRgbColor( Rgb::Black, 50 );
        m_wifiLed.turnOff();
    }
    m_led.turnOff();
    m_wifiLed.turnOff();
}
