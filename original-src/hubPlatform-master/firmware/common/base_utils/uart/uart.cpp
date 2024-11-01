#include "base_utils/uart/uart.h"

#include <stdexcept>
#include <sstream>

#include <cstdint>
#include <cstring>
#include <iostream>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

namespace {

static constexpr int UartInvalidValue = -1;
}

Uart::Uart( const std::string& tty, const bool nonBlocking, bool hwFlowControl, bool throwException )
: m_ttyPath( tty )
, m_ttyFd( UartInvalidValue )
, m_isOpened( false )
, m_isConfigured( false )
, m_isNonBlocking( nonBlocking ) {
    m_isOpened = open( m_isNonBlocking );
    if( m_isOpened ) {
        m_isConfigured = configure( hwFlowControl );
    }
    if( !isInitialized() ) {
        std::string error = "Failed to initialize UART " + m_ttyPath + "\n";

        std::cerr << error << std::flush;
        std::cout << error << std::flush;

        if( throwException ) {
            throw std::runtime_error( error );
        }
        else {
            exit( EXIT_FAILURE );
        }
    }
}

Uart::~Uart() {
    if( isInitialized() ) {
        close();
    }
}

bool Uart::isInitialized() const {
    return m_isOpened && m_isConfigured;
}

int Uart::getInputBufferBitsCount( int& val ) {
    return ioctl( m_ttyFd, FIONREAD, &val );
}

int Uart::read( void* data, size_t size ) {
    if( isInitialized() ) {
        auto readResult = ::read( m_ttyFd, data, size );
        if( readResult < 0 && !( m_isNonBlocking && errno == EAGAIN ) ) {
            // print errno string - skip for non-blocking read errors ( EAGAIN )
            std::cerr << "Failed to read from UART: " << strerror( errno ) << std::endl;
        }
        return readResult;
    }
    std::cerr << "Failed to read from UART " << m_ttyPath << std::endl;
    return -1;
}

int Uart::write( const void* data, size_t size ) {
    if( isInitialized() ) {
        return ::write( m_ttyFd, reinterpret_cast< const char* >( data ), size );
    }
    std::cerr << "Failed to write to UART " << m_ttyPath << std::endl;
    return -1;
}

bool Uart::isIoOk( int status ) {
    return status >= 0;
}

int Uart::flush() {
    int result = -1;
    if( m_ttyFd >= 0 ) {
        result = tcflush( m_ttyFd, TCIOFLUSH );
    }
    return result;
}

bool Uart::open( const bool nonBloking ) {
    m_ttyFd = ::open( m_ttyPath.c_str(), O_RDWR | O_NOCTTY | O_CLOEXEC | O_NDELAY );
    if( m_ttyFd < 0 ) {
        std::cerr << "Failed to open TTY device, errno: " << (int) errno << std::endl;
        return false;
    }

    if( !nonBloking && fcntl( m_ttyFd, F_SETFL, fcntl( m_ttyFd, F_GETFL ) & ~O_NONBLOCK ) < 0 ) {
        std::cerr << "Failed to set TTY device to block mode, errno: " << (int) errno << std::endl;
        closeAndClearFd();
        return false;
    }

    if( !isatty( m_ttyFd ) ) {
        std::cerr << "Failed to locate TTY device" << std::endl;
        closeAndClearFd();
        return false;
    }

    return true;
}

bool Uart::configure( bool hwFlowControl ) {
    termios uartConfig;
    if( tcgetattr( m_ttyFd, &uartConfig ) < 0 ) {
        std::cerr << "Failed to get TTY device config" << std::endl;
        closeAndClearFd();
        return false;
    }
    std::memcpy( &m_uartConfigBackup, &uartConfig,
                 sizeof( struct termios ) );  // backup current TTY config
    std::memset( &uartConfig, 0, sizeof( uartConfig ) );
    // 115200 8N1
    cfmakeraw( &uartConfig );
    if( cfsetispeed( &uartConfig, B115200 ) < 0 || cfsetospeed( &uartConfig, B115200 ) < 0 ) {
        std::cerr << "Failed to set TTY device baud rate" << std::endl;
        closeAndClearFd();
        return false;
    }
    uartConfig.c_cflag &= ~CSIZE;
    uartConfig.c_cflag |= CS8;
    uartConfig.c_cflag &= ~( PARENB | PARODD );
    uartConfig.c_cflag |= 0;
    uartConfig.c_cflag &= ~CSTOPB;
    uartConfig.c_cflag |= 0;
    if( hwFlowControl ) {
        uartConfig.c_cflag |= CRTSCTS;
    }
    else {
        uartConfig.c_cflag &= ~CRTSCTS;
    }
    uartConfig.c_cflag |= 0;
    uartConfig.c_cflag |= ( CLOCAL | CREAD );
    uartConfig.c_cc[ VMIN ] = 1;
    uartConfig.c_cc[ VTIME ] = 0;

    if( tcsetattr( m_ttyFd, TCSANOW, &uartConfig ) < 0 ) {
        std::cerr << "Failed to set TTY device attributes" << std::endl;
        closeAndClearFd();
        return false;
    }

    return true;
}

void Uart::close() {
    tcsetattr( m_ttyFd, TCSAFLUSH, &m_uartConfigBackup );
    closeAndClearFd();
}

void Uart::closeAndClearFd() {
    ::close( m_ttyFd );
    m_ttyFd = UartInvalidValue;
    m_isOpened = false;
    m_isConfigured = false;
}
