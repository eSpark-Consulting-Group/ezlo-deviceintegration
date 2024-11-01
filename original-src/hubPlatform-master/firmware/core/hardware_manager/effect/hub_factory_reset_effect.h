#pragma once

#include "hardware_manager/effect/led_color_effect.h"

void hubFactoryResetEffectPreset( LedColor& led );

class HubFactoryResetEffect final : public LedColorEffect {
public:
    HubFactoryResetEffect( LedColor& led );
    void effect() override;
};
