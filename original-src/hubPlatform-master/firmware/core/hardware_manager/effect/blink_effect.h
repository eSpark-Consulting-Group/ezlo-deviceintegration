#pragma once

#include "hardware_manager/effect/led_color_effect.h"

class BlinkEffect final : public LedColorEffect {
public:
    BlinkEffect( LedColor& led );
    void effect() override;
    void setBlinkParameters( Rgb color, int count, int delay, int gradual );

private:
    Rgb m_color;
    int m_count;
    int m_delay;
    int m_gradual;
};
