#pragma once

#include "hardware_manager/effect/led_color_effect.h"

void updateEzloFWEffectPreset( LedColor& led );

class UpdateEzloFWEffect final : public LedColorEffect {
public:
    UpdateEzloFWEffect( LedColor& led );
    void effect() override;
};
