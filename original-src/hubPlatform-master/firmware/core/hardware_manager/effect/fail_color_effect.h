#pragma once

#include "hardware_manager/effect/led_color_effect.h"

class FailColorEffect final : public LedColorEffect {
public:
    FailColorEffect( LedColor& led );
    void effect() override;
};