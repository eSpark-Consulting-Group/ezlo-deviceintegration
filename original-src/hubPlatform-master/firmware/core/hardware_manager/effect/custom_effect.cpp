#include "hardware_manager/effect/custom_effect.h"
#include <iostream>

CustomEffect::CustomEffect( LedColor& led, const Color color, const BlinkingType type )
: LedColorEffect( led )
, m_type( type )
, m_color( color ) {}

Rgb CustomEffect::getColor() {
    Rgb color;
    if( m_color == Color::Red ) {
        color = Rgb::Red;
    }
    else if( m_color == Color::Green ) {
        color = Rgb::Green;
    }
    else if( m_color == Color::Blue ) {
        color = Rgb::Blue;
    }
    else if( m_color == Color::Yellow ) {
        color = Rgb::Yellow;
    }
    else if( m_color == Color::Orange ) {
        color = Rgb::LightOrange;
    }
    else if( m_color == Color::Cyan ) {
        color = Rgb::Cyan;
    }
    else if( m_color == Color::Magenta ) {
        color = Rgb::Magenta;
    }
    else if( m_color == Color::White ) {
        color = Rgb::White;
    }
    else {
        color = Rgb::Black;
    }

    return color;
}

void CustomEffect::effect() {
    const auto& color = getColor();

    if( m_type == BlinkingType::On ) {
        gradualRgbColor( color );
    }
    else if( m_type == BlinkingType::Blink ) {
        blinkingRgbColor( color, -1, 80, 16 );
    }
    else if( m_type == BlinkingType::Breathe ) {
        wavingRgbColor( color, Rgb::Black, -1, 1000, 132 );
    }
}
