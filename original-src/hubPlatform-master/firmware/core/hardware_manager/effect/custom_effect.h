#pragma once

#include "hardware_manager/effect/led_color_effect.h"

class CustomEffect final : public LedColorEffect {
public:
    CustomEffect( LedColor& led, const Color color, const BlinkingType type );
    void effect() override;

private:
    Rgb getColor();

private:
    Color m_color;
    BlinkingType m_type;
};
