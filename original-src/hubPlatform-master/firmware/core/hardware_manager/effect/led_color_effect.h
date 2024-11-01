#pragma once
#include "hardware_manager/led/led_color.h"
#include "hardware_manager/effect/effect.h"
#include "base_utils/i2ccontrol.h"

class LedColorEffect : public Effect {
public:
    LedColorEffect( LedColor& led );
    virtual ~LedColorEffect();

protected:
    LedColor& m_led;
    void gradualRgbColor( const Rgb& color );
    void gradualRgbColor( const Rgb& color, int steps );
    void gradualRgbColor( const Rgb& color, int steps, int delay );
    void blinkingRgbColor( const Rgb& color, int count, int delay );
    void blinkingRgbColor( const Rgb& color, int count, int delay, int steps );
    void wavingRgbColor( const Rgb& color1, const Rgb& color2, int count, int delay );
    void wavingRgbColor( const Rgb& color1, const Rgb& color2, int count, int delay, int steps );

private:
    Rgb calculateCurrentColor( const Rgb& currentColor, const Rgb& targetColor, int currentStep, int totalSteps ) const;
    i2c::Byte calculateCurrentColor( i2c::Byte currentColor,
                                     i2c::Byte targetColor,
                                     int currentStep,
                                     int totalSteps ) const;
    void delay( int delay );
};
