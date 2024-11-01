#include "utils/serial_port/serial_port_impl.h"
#include "utils/serial_port/serial_port_configuration.h"

#include <fcntl.h>
#include <unistd.h>

namespace TwoGig {

SerialPortImpl::SerialPortImpl( const std::string& devicePath, const ConfigurationSettings& settings )
: m_devicePath( devicePath )
, m_settings( settings )
, m_fileDescriptor( -1 ) {}

bool SerialPortImpl::open() {
    m_fileDescriptor = ::open( m_devicePath.c_str(), O_RDWR | O_NOCTTY | O_CLOEXEC );
    if( m_fileDescriptor < 0 )
        return false;

    if( !isatty( m_fileDescriptor ) ) {
        reset();
        return false;
    }

    if( !saveCurrentConfiguration() ) {
        reset();
        return false;
    }

    if( !configure() ) {
        reset();
        return false;
    }

    return true;
}

void SerialPortImpl::close() {
    loadPreviousConfiguration();
    reset();
}

bool SerialPortImpl::readByte( uint8_t& buffer ) const {
    return ::read( m_fileDescriptor, &buffer, 1 ) == 1;
}

bool SerialPortImpl::configure() const {
    termios config = getCustomConfiguration( m_previousConfig, m_settings );

    // Block reading calls for at most 1 second
    config.c_cc[ VMIN ] = 0;
    config.c_cc[ VTIME ] = 10;

    return tcsetattr( m_fileDescriptor, TCSAFLUSH, &config ) == 0;
}

bool SerialPortImpl::saveCurrentConfiguration() {
    return tcgetattr( m_fileDescriptor, &m_previousConfig ) == 0;
}

bool SerialPortImpl::loadPreviousConfiguration() const {
    return tcsetattr( m_fileDescriptor, TCSAFLUSH, &m_previousConfig ) == 0;
}

void SerialPortImpl::reset() {
    ::close( m_fileDescriptor );
    m_fileDescriptor = -1;
}

}  // namespace TwoGig
