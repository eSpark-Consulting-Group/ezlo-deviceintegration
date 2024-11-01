#include "hardware_manager/led/led_color_gpio.h"
#include <math.h>
#include <fstream>

LedColorGPIO::LedColorGPIO( std::string name )
: LedColor( std::move( name ) )
, m_defaultPeriod( 20000000 )
, m_redPath( "/sys/class/pwm/gpio_pwm:15/" )
, m_greenPath( "/sys/class/pwm/gpio_pwm:16/" )
, m_bluePath( "/sys/class/pwm/gpio_pwm:17/" ) {
    initPeriod();
}

LedColorGPIO::LedColorGPIO( std::string name,
                            int pwmPeriod,
                            std::string redPath,
                            std::string greenPath,
                            std::string bluePath )
: LedColor( std::move( name ) )
, m_defaultPeriod( pwmPeriod )
, m_redPath( std::move( redPath ) )
, m_greenPath( std::move( greenPath ) )
, m_bluePath( std::move( bluePath ) ) {
    initPeriod();
}

LedColorGPIO::~LedColorGPIO() {
    turnOff();
}

void LedColorGPIO::initPeriod() {
    setPeriod( m_redPath );
    setPeriod( m_greenPath );
    setPeriod( m_bluePath );
}

bool LedColorGPIO::turnOn() {
    setColor( m_redPath, getColor().red );
    setColor( m_greenPath, getColor().green );
    setColor( m_bluePath, getColor().blue );
    state = LedState::LedOn;
    return true;
}

bool LedColorGPIO::setColor( std::string path, i2c::Byte color ) {
    std::ofstream sysfsIface( path + "/duty_ns" );
    sysfsIface << rgbToPwm( color ) << std::endl;
    sysfsIface.close();
    return true;
}

bool LedColorGPIO::setPeriod( std::string path ) {
    std::ofstream sysfsIface( path + "/period_ns" );
    sysfsIface << m_defaultPeriod << std::endl;
    sysfsIface.close();
    return true;
}

bool LedColorGPIO::turnOff() {
    setColor( m_redPath, 0 );
    setColor( m_greenPath, 0 );
    setColor( m_bluePath, 0 );
    state = LedState::LedOff;
    return true;
}

bool LedColorGPIO::turnBlinking( int, int ) {
    return false;
    /*
    I need to research how to do this on the
    A20 big Ezlo led
    */
}

int LedColorGPIO::rgbToPwm( i2c::Byte value ) {
    return static_cast< int >( round( ( m_defaultPeriod - 1.0 ) * pow( getColor().normalize( (int) value ), 2 ) ) );
}
