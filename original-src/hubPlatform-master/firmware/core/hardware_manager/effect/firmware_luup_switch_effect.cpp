#include "hardware_manager/effect/firmware_luup_switch_effect.h"

#include <thread>
#include <chrono>

FirmwareLuupSwitchEffect::FirmwareLuupSwitchEffect( Led& internetLed, Led& wifiLed, Led& zwaveLed, Led& serviceLed )
: m_internetLed( internetLed )
, m_wifiLed( wifiLed )
, m_zwaveLed( zwaveLed )
, m_serviceLed( serviceLed ) {}

void FirmwareLuupSwitchEffect::effect() {
    while( isRunning() ) {
        m_internetLed.turnOn();
        m_wifiLed.turnOn();
        m_zwaveLed.turnOn();
        m_serviceLed.turnOn();

        std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );

        m_internetLed.turnOff();
        m_wifiLed.turnOff();
        m_zwaveLed.turnOff();
        m_serviceLed.turnOff();

        std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
    }

    m_internetLed.turnOff();
    m_wifiLed.turnOff();
    m_zwaveLed.turnOff();
    m_serviceLed.turnOff();
}
