#include "hardware_manager/led/led_color.h"

LedColor::LedColor( std::string name )
: Led( std::move( name ) ) {}

LedColor::LedColor( std::string name, Rgb color )
: Led( std::move( name ) )
, m_color( color ) {}

Rgb LedColor::getColor() const {
    return m_color;
}

void LedColor::setColor( Rgb color ) {
    m_color = color;
}