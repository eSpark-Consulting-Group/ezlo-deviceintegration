#pragma once

#include "hardware_manager/effect/led_color_effect.h"

class OkColorEffect final : public LedColorEffect {
public:
    OkColorEffect( LedColor& led );
    void effect() override;
};