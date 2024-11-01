#pragma once

#include "hardware_manager/led/led.h"
#include "hardware_manager/utils/rgb.h"
#include <string>

class LedColor : public Led {
public:
    LedColor( std::string name );
    LedColor( std::string name, Rgb color );
    virtual ~LedColor() = default;
    virtual void setColor( Rgb color );
    virtual Rgb getColor() const;

protected:
    Rgb m_color;
};