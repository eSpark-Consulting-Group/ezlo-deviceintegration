#pragma once

#include "hardware_manager/led/led_color.h"
#include "base_utils/i2ccontrol.h"

#include <memory>
#include <string>

class LedColorPwm : public LedColor {
public:
    LedColorPwm( std::string name );
    LedColorPwm( std::string name,
                 const std::string& devPath,
                 i2c::Byte redAddr,
                 i2c::Byte greenAddr,
                 i2c::Byte blueAddr );
    ~LedColorPwm();

    bool turnOn() override;
    bool turnOff() override;
    bool turn( bool enable ) override;
    bool turnBlinking( int msOff, int msOn ) override;
    void setColor( Rgb color ) override;

private:
    void initBus();

private:
    std::unique_ptr< i2c::I2cControl > m_i2cControl;
    const i2c::Byte m_redAddr;
    const i2c::Byte m_greenAddr;
    const i2c::Byte m_blueAddr;
};
