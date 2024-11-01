#pragma once

#include "hardware_manager/effect/led_color_effect.h"

class DefaultRecoveryEffect final : public LedColorEffect {
public:
    DefaultRecoveryEffect( LedColor& led );
    void effect() override;

private:
    int m_counter;
};