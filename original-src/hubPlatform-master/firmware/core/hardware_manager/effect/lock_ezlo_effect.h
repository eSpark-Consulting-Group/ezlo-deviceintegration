#pragma once

#include "hardware_manager/effect/led_color_effect.h"

class LockEzloEffect final : public LedColorEffect {
public:
    LockEzloEffect( LedColor& led );
    void effect() override;
};