#pragma once

#include "hardware_manager/led/led_effect.h"
#include "hardware_manager/led/led_sysfs.h"

class WifiConnectingEffect final : public Effect {
public:
    WifiConnectingEffect( Led& wifiLed );
    void effect() override;

private:
    Led& m_wifiLed;
};
