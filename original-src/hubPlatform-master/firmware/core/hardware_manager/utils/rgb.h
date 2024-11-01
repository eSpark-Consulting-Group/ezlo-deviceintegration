#pragma once

#include "base_utils/i2ccontrol.h"
#include "hardware_manager/utils/domain.h"

class Rgb final : public Domain {
public:
    using Domain::normalize;

    Rgb();
    Rgb( i2c::Byte red, i2c::Byte green, i2c::Byte blue );
    Rgb( const Rgb& src ) = default;
    Rgb& operator=( const Rgb& other ) = default;

    float normalize( int x ) override;

public:
    i2c::Byte red;
    i2c::Byte green;
    i2c::Byte blue;

public:
    static const i2c::Byte maxColor;
    static const Rgb White;
    static const Rgb Black;
    static const Rgb Red;
    static const Rgb Green;
    static const Rgb Blue;
    static const Rgb Yellow;
    static const Rgb Magenta;
    static const Rgb Cyan;
    static const Rgb Silver;
    static const Rgb DarkGray;
    static const Rgb Pink;
    static const Rgb LightOrange;
    static const Rgb LightBlue;
};