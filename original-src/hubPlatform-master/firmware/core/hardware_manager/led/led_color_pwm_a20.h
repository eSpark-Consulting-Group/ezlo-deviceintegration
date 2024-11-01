#pragma once

#include "hardware_manager/led/led_color.h"
#include "base_utils/i2ccontrol.h"
#include <memory>
#include <string>

class LedColorPwmA20 final : public LedColor {
public:
    LedColorPwmA20( std::string name );
    LedColorPwmA20( std::string name,
                    const std::string& devPath,
                    int pwmPeriod,
                    i2c::Byte pwmAddr,
                    i2c::Byte redAddr,
                    i2c::Byte greenAddr,
                    i2c::Byte blueAddr,
                    i2c::Byte initSeq );
    ~LedColorPwmA20();
    bool turnOn() override;
    bool turnOff() override;
    bool turn( bool enable ) override;
    bool turnBlinking( int msOff, int msOn ) override;

private:
    int rgbToPwm( i2c::Byte value );
    void initBus();

private:
    std::unique_ptr< i2c::I2cControl > m_i2cControl;
    int m_defaultPeriod;
    i2c::Byte m_pwmAddr;
    i2c::Byte m_redAddr;
    i2c::Byte m_greenAddr;
    i2c::Byte m_blueAddr;
    i2c::Byte m_initSeq;
};
