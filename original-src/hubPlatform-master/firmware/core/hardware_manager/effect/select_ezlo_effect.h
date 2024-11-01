#pragma once

#include "hardware_manager/effect/led_color_effect.h"

class SelectEzloEffect final : public LedColorEffect {
public:
    SelectEzloEffect( LedColor& led );
    void effect() override;
};