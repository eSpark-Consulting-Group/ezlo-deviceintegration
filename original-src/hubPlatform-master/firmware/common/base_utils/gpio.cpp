#include "base_utils/gpio.h"
#include <fstream>

namespace common {

Gpio::Gpio( int gpioNumber ) {
    setGpio( gpioNumber );
}

bool Gpio::setGpio( int gpioNumber ) {
    m_gpioNumber = "gpio" + std::to_string( gpioNumber );
    if( isGpioSet( m_gpioNumber ) ) {
        return true;
    }
    const std::string filename = std::string( GpioPath ) + "export";
    std::ofstream file( filename.c_str() );
    if( !file.good() ) {
        return false;
    }
    file << gpioNumber;
    return true;
}

bool Gpio::isGpioSet( const std::string& gpioNumber ) {
    const std::string filename = GpioPath + gpioNumber + "/value";
    std::ifstream file( filename.c_str() );
    return file.good();
}

bool Gpio::setEdgeDetecting( GpioEdge edge ) {
    const std::string filename = std::string( GpioPath ) + m_gpioNumber + "/edge";
    std::ofstream file( filename.c_str() );
    if( !file.good() ) {
        return false;
    }
    switch( edge ) {
    case GpioEdge::RISING:
        file << "rising";
        break;
    case GpioEdge::FALLING:
        file << "falling";
        break;
    case GpioEdge::BOTH:
        file << "both";
        break;
    case GpioEdge::NONE:
        file << "none";
        break;
    }
    return true;
}

GpioState Gpio::getState() const {
    const std::string filename = GpioPath + m_gpioNumber + "/value";
    std::ifstream file( filename.c_str() );
    if( !file.good() ) {
        return GpioState::UNDEFINED;
    }
    char state;
    file >> state;
    if( file.bad() ) {
        return GpioState::UNDEFINED;
    }
    return static_cast< GpioState >( state - '0' );
}

bool Gpio::setState( GpioState state ) {
    if( m_direction == GpioDirection::IN ) {
        return false;
    }
    const std::string filename = GpioPath + m_gpioNumber + "/value";
    std::ofstream file( filename.c_str() );
    if( !file.good() ) {
        return false;
    }
    if( state == GpioState::ON ) {
        file << 1;
    }
    else {
        file << 0;
    }
    return true;
}

bool Gpio::setDirection( GpioDirection direction ) {
    m_direction = direction;
    const std::string filename = std::string( GpioPath ) + m_gpioNumber + "/direction";
    std::ofstream file( filename.c_str() );
    if( !file.good() ) {
        return false;
    }
    if( direction == GpioDirection::IN ) {
        file << "in";
    }
    else {
        file << "out";
    }
    return true;
}

}  // namespace common
