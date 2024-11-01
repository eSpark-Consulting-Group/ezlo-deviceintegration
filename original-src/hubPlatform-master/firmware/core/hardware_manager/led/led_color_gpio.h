#pragma once

#include "hardware_manager/led/led_color.h"
#include "base_utils/i2ccontrol.h"
#include <string>

class LedColorGPIO final : public LedColor {
public:
    LedColorGPIO( std::string name );
    LedColorGPIO( std::string name, int pwmPeriod, std::string redPath, std::string greenPath, std::string bluePath );
    ~LedColorGPIO();
    bool turnOn() override;
    bool turnOff() override;
    bool turnBlinking( int msOff, int msOn ) override;

private:
    int rgbToPwm( i2c::Byte value );
    bool setPeriod( std::string path );
    bool setColor( std::string path, i2c::Byte color );
    void initPeriod();

private:
    int m_defaultPeriod;
    std::string m_redPath;
    std::string m_greenPath;
    std::string m_bluePath;
};