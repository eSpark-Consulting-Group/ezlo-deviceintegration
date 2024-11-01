#pragma once

#include "hardware_manager/effect/led_color_effect.h"

class BatteryDischargingLowEffect final : public LedColorEffect {
public:
    explicit BatteryDischargingLowEffect( LedColor& led );
    void effect() override;
};
