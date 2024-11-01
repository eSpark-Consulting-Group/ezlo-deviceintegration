#include "hardware_manager/effect/led_color_effect.h"
#include <thread>
#include <chrono>
#include <math.h>

LedColorEffect::LedColorEffect( LedColor& led )
: m_led( led ) {}

LedColorEffect::~LedColorEffect() {}

void LedColorEffect::gradualRgbColor( const Rgb& color ) {
    gradualRgbColor( color, 100 );
}

void LedColorEffect::gradualRgbColor( const Rgb& color, int steps ) {
    gradualRgbColor( color, steps, 0 );
}

/*!
 * Performs graduate color change effect from current color to specified
 *
 * @param [in] color target color
 * @param [in] steps number steps needed for color change, each step delay is 10 ms
 * @param [in] delay delay between steps, specified in tens of milliseconds ( delay = 10 equals to 100 ms)
 */
void LedColorEffect::gradualRgbColor( const Rgb& color, int steps, int delay ) {
    Rgb currentColor( m_led.getColor() );
    Rgb targetColor( color );
    for( int step = 0; isRunning() && step < steps; ++step ) {
        m_led.setColor( calculateCurrentColor( currentColor, targetColor, step, steps ) );
        m_led.turnOn();
        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
    }
    this->delay( delay );
}

void LedColorEffect::blinkingRgbColor( const Rgb& color, int count, int delay ) {
    blinkingRgbColor( color, count, delay, 10 );
}

/*!
 * Performs colorful blinking effect
 *
 * @param [in] color blinking color
 * @param [in] count number of full blinking cycles ( ON/OFF ), -1 for infinite
 * @param [in] delay delay between blinking states ( ON-delay-OFF )
 * @param [in] steps graduate color number of steps
 */
void LedColorEffect::blinkingRgbColor( const Rgb& color, int count, int delay, int steps ) {
    wavingRgbColor( Rgb::Black, color, count, delay, steps );
}

void LedColorEffect::wavingRgbColor( const Rgb& color1, const Rgb& color2, int count, int delay ) {
    wavingRgbColor( color1, color2, count, delay, 10 );
}

/*!
 * Performs colorful waving effect
 *
 * @param [in] color1 first waving color
 * @param [in] color2 second waving color
 * @param [in] count number of full blinking cycles ( ON/OFF ), -1 for infinite
 * @param [in] delay delay between blinking states ( ON-delay-OFF )
 * @param [in] steps graduate color number of steps
 */
void LedColorEffect::wavingRgbColor( const Rgb& color1, const Rgb& color2, int count, int delay, int steps ) {
    int limit = count * 2;
    for( int i = 1; isRunning() && ( limit < 0 || i <= limit ); ++i ) {
        if( i % 2 ) {
            gradualRgbColor( color1, steps );
        }
        else {
            gradualRgbColor( color2, steps );
        }
        this->delay( delay );
    }
}
i2c::Byte LedColorEffect::calculateCurrentColor( i2c::Byte currentColor,
                                                 i2c::Byte targetColor,
                                                 int currentStep,
                                                 int totalSteps ) const {
    return static_cast< i2c::Byte >( currentColor + ( targetColor - currentColor ) * currentStep / totalSteps );
}

Rgb LedColorEffect::calculateCurrentColor( const Rgb& currentColor,
                                           const Rgb& targetColor,
                                           int currentStep,
                                           int totalSteps ) const {
    Rgb rez;
    rez.red = calculateCurrentColor( currentColor.red, targetColor.red, currentStep, totalSteps );
    rez.green = calculateCurrentColor( currentColor.green, targetColor.green, currentStep, totalSteps );
    rez.blue = calculateCurrentColor( currentColor.blue, targetColor.blue, currentStep, totalSteps );
    return rez;
}

void LedColorEffect::delay( int delay ) {
    while( isRunning() && delay > 0 ) {
        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
        delay -= 10;
    }
}
