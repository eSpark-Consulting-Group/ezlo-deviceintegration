#pragma once

#include "hardware_manager/effect/led_color_effect.h"

class BatteryDischargingCriticalEffect final : public LedColorEffect {
public:
    explicit BatteryDischargingCriticalEffect( LedColor& led );
    void effect() override;
};
