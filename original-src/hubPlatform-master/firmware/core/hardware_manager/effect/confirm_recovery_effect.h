#pragma once

#include "hardware_manager/effect/led_color_effect.h"

class ConfirmRecoveryEffect final : public LedColorEffect {
public:
    ConfirmRecoveryEffect( LedColor& led );
    void effect() override;
};
