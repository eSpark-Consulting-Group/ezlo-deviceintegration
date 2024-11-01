#pragma once

#include "hardware_manager/effect/led_color_effect.h"

class DefaultColorEffect final : public LedColorEffect {
public:
    DefaultColorEffect( LedColor& led );
    void effect() override;
};