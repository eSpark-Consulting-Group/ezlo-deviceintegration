#pragma once

#include "hardware_manager/effect/led_color_effect.h"

class EzloFoundEffect final : public LedColorEffect {
public:
    EzloFoundEffect( LedColor& led );
    void effect() override;
};
