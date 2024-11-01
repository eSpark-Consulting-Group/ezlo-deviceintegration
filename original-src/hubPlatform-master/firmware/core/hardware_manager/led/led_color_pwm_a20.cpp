
#include "hardware_manager/led/led_color_pwm_a20.h"
#include <math.h>

LedColorPwmA20::LedColorPwmA20( std::string name )
: LedColor( std::move( name ) )
, m_i2cControl( std::unique_ptr< i2c::I2cControl >( new i2c::I2cControl( "/dev/i2c-2" ) ) )
, m_defaultPeriod( 32 )
, m_pwmAddr( 0x38 )
, m_redAddr( 0x80 )
, m_greenAddr( 0x60 )
, m_blueAddr( 0x40 )
, m_initSeq( 0x3F ) {
    initBus();
}

LedColorPwmA20::LedColorPwmA20( std::string name,
                                const std::string& devPath,
                                int pwmPeriod,
                                i2c::Byte pwmAddr,
                                i2c::Byte redAddr,
                                i2c::Byte greenAddr,
                                i2c::Byte blueAddr,
                                i2c::Byte initSeq )
: LedColor( std::move( name ) )
, m_i2cControl( std::unique_ptr< i2c::I2cControl >( new i2c::I2cControl( devPath ) ) )
, m_defaultPeriod( pwmPeriod )
, m_pwmAddr( pwmAddr )
, m_redAddr( redAddr )
, m_greenAddr( greenAddr )
, m_blueAddr( blueAddr )
, m_initSeq( initSeq ) {
    initBus();
}

LedColorPwmA20::~LedColorPwmA20() {
    m_i2cControl.reset();
}

void LedColorPwmA20::initBus() {
    m_i2cControl->write( m_pwmAddr, { m_initSeq } );
    turnOff();
}

bool LedColorPwmA20::turnOn() {
    m_i2cControl->write( m_pwmAddr, { static_cast< i2c::Byte >( m_redAddr + rgbToPwm( getColor().red ) ) } );
    m_i2cControl->write( m_pwmAddr, { static_cast< i2c::Byte >( m_greenAddr + rgbToPwm( getColor().green ) ) } );
    m_i2cControl->write( m_pwmAddr, { static_cast< i2c::Byte >( m_blueAddr + rgbToPwm( getColor().blue ) ) } );
    state = LedState::LedOn;
    return true;
}

bool LedColorPwmA20::turnOff() {
    m_i2cControl->write( m_pwmAddr, { static_cast< i2c::Byte >( m_redAddr ) } );
    m_i2cControl->write( m_pwmAddr, { static_cast< i2c::Byte >( m_greenAddr ) } );
    m_i2cControl->write( m_pwmAddr, { static_cast< i2c::Byte >( m_blueAddr ) } );
    state = LedState::LedOff;
    return true;
}

bool LedColorPwmA20::turn( bool enable ) {
    return ( enable ) ? turnOn() : turnOff();
}

bool LedColorPwmA20::turnBlinking( int, int ) {
    return false;
    /*
    I need to research how to do this on the
    A20 big Ezlo led
    */
}

int LedColorPwmA20::rgbToPwm( i2c::Byte value ) {
    return static_cast< int >( round( ( m_defaultPeriod - 1.0 ) * pow( getColor().normalize( (int) value ), 2 ) ) );
}
