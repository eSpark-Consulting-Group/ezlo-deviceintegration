#pragma once

#include "hardware_manager/effect/led_color_effect.h"
#include "hardware_manager/led/led_sysfs.h"

class WifiWpsColorEffect final : public LedColorEffect {
public:
    WifiWpsColorEffect( LedColor& led, Led& wifiLed );
    void effect() override;

private:
    Led& m_wifiLed;
};
