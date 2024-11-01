#pragma once

#include "hardware_manager/effect/led_color_effect.h"

class HubPowerOffEffect final : public LedColorEffect {
public:
    HubPowerOffEffect( LedColor& led );
    void effect() override;
};
