#pragma once

#include "hardware_manager/effect/led_color_effect.h"

class BatteryDischargingEffect final : public LedColorEffect {
public:
    explicit BatteryDischargingEffect( LedColor& led );
    void effect() override;
};
