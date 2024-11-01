#include "utils/serial_port/serial_port_configuration.h"

#include <cstring>

namespace TwoGig {

static void setBaudRate( termios& config, BaudRate::Enum speed ) {
    switch( speed ) {
    case BaudRate::Baud_0:
        cfsetspeed( &config, B0 );
        break;

    case BaudRate::Baud_50:
        cfsetspeed( &config, B50 );
        break;

    case BaudRate::Baud_75:
        cfsetspeed( &config, B75 );
        break;

    case BaudRate::Baud_110:
        cfsetspeed( &config, B110 );
        break;

    case BaudRate::Baud_134:
        cfsetspeed( &config, B134 );
        break;

    case BaudRate::Baud_150:
        cfsetspeed( &config, B150 );
        break;

    case BaudRate::Baud_200:
        cfsetspeed( &config, B200 );
        break;

    case BaudRate::Baud_300:
        cfsetspeed( &config, B300 );
        break;

    case BaudRate::Baud_600:
        cfsetspeed( &config, B600 );
        break;

    case BaudRate::Baud_1200:
        cfsetspeed( &config, B1200 );
        break;

    case BaudRate::Baud_1800:
        cfsetspeed( &config, B1800 );
        break;

    case BaudRate::Baud_2400:
        cfsetspeed( &config, B2400 );
        break;

    case BaudRate::Baud_4800:
        cfsetspeed( &config, B4800 );
        break;

    case BaudRate::Baud_9600:
        cfsetspeed( &config, B9600 );
        break;

    case BaudRate::Baud_19200:
        cfsetspeed( &config, B19200 );
        break;

    case BaudRate::Baud_38400:
        cfsetspeed( &config, B38400 );
        break;
    }
}

static void setParity( termios& config, Parity::Enum parity ) {
    config.c_cflag &= ~( PARENB | PARODD );

    switch( parity ) {
    case Parity::Odd:
        config.c_cflag |= ( PARENB | PARODD );
        break;

    case Parity::Even:
        config.c_cflag |= PARENB;
        break;

    default:
        break;
    }
}

static void setCharacterSize( termios& config, CharacterSize::Enum characterSize ) {
    config.c_cflag &= ~CSIZE;

    switch( characterSize ) {
    case CharacterSize::CS_5:
        config.c_cflag |= CS5;
        break;

    case CharacterSize::CS_6:
        config.c_cflag |= CS6;
        break;

    case CharacterSize::CS_7:
        config.c_cflag |= CS7;
        break;

    case CharacterSize::CS_8:
        config.c_cflag |= CS8;
        break;
    }
}

static void setStopBitsCount( termios& config, StopBitsCount::Enum stopBitsCount ) {
    switch( stopBitsCount ) {
    case StopBitsCount::One:
        config.c_cflag &= ~CSTOPB;
        break;

    case StopBitsCount::Two:
        config.c_cflag |= CSTOPB;
        break;
    }
}

static void setHardwareFlowControl( termios& config, bool isEnabled ) {
    if( isEnabled )
        config.c_cflag |= CRTSCTS;
    else
        config.c_cflag &= ~CRTSCTS;
}

termios getCustomConfiguration( const termios& prototype, const ConfigurationSettings& settings ) {
    termios config;
    std::memcpy( &config, &prototype, sizeof( termios ) );

    cfmakeraw( &config );

    setBaudRate( config, settings.baudRate );
    setCharacterSize( config, settings.characterSize );
    setParity( config, settings.parity );
    setStopBitsCount( config, settings.stopBitsCount );
    setHardwareFlowControl( config, settings.hardwareFlowControlEnabled );

    return config;
}

}  // namespace TwoGig
