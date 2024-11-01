#pragma once

#include "hardware_manager/effect/led_color_effect.h"

void hubSoftResetEffectPreset( LedColor& led );

class HubSoftResetEffect final : public LedColorEffect {
public:
    HubSoftResetEffect( LedColor& led );
    void effect() override;
};
