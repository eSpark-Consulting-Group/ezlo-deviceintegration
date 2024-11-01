#pragma once

#include "hardware_manager/effect/led_color_effect.h"

void hubRebootEffectPreset( LedColor& led );

class HubRebootEffect final : public LedColorEffect {
public:
    HubRebootEffect( LedColor& led );
    void effect() override;
};
