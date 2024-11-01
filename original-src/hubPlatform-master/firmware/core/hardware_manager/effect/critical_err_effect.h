#pragma once

#include "hardware_manager/effect/led_color_effect.h"

class CriticalErrEffect final : public LedColorEffect {
public:
    CriticalErrEffect( LedColor& led );
    void effect() override;

private:
    int m_counter;
};