#include "hardware_manager/utils/rgb.h"

Rgb::Rgb()
: red( 0 )
, green( 0 )
, blue( 0 ) {}

Rgb::Rgb( i2c::Byte red, i2c::Byte green, i2c::Byte blue ) {
    this->red = red;
    this->green = green;
    this->blue = blue;
}

float Rgb::normalize( int x ) {
    return normalize( x, static_cast< int >( Rgb::maxColor ) );
}

const i2c::Byte Rgb::maxColor( 255 );
const Rgb Rgb::White( 255, 255, 255 );
const Rgb Rgb::Black( 0, 0, 0 );
const Rgb Rgb::Red( 255, 0, 0 );
const Rgb Rgb::Green( 0, 255, 0 );
const Rgb Rgb::Blue( 0, 0, 255 );
const Rgb Rgb::Yellow( 255, 255, 0 );
const Rgb Rgb::Magenta( 255, 0, 255 );
const Rgb Rgb::Cyan( 0, 255, 255 );
const Rgb Rgb::Silver( 192, 192, 192 );
const Rgb Rgb::DarkGray( 77, 77, 77 );
const Rgb Rgb::Pink( 190, 63, 190 );
const Rgb Rgb::LightOrange( 255, 130, 0 );
const Rgb Rgb::LightBlue( 50, 255, 255 );