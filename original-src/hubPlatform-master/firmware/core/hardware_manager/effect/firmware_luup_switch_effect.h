#pragma once

#include "hardware_manager/led/led_effect.h"
#include "hardware_manager/led/led_sysfs.h"

class FirmwareLuupSwitchEffect final : public Effect {
public:
    FirmwareLuupSwitchEffect( Led& internetLed, Led& wifiLed, Led& zwaveLed, Led& serviceLed );
    void effect() override;

private:
    Led& m_internetLed;
    Led& m_wifiLed;
    Led& m_zwaveLed;
    Led& m_serviceLed;
};
