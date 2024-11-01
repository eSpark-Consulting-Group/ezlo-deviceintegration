#include "hardware_manager/effect/wifi_connecting_effect.h"
#include <thread>
#include <chrono>

WifiConnectingEffect::WifiConnectingEffect( Led& wifiLed )
: m_wifiLed( wifiLed ) {}

void WifiConnectingEffect::effect() {
    while( isRunning() ) {
        m_wifiLed.turnOn();
        std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
        m_wifiLed.turnOff();
        std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
    }
    m_wifiLed.turnOff();
}
