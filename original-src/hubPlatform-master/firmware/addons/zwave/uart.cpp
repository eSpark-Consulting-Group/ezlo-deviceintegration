#include "zwave/uart.h"

#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"

#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

namespace HubPlatform {
namespace Zwave {

constexpr int UartInvalidValue = -1;

Uart::Uart( const std::string& tty )
: m_ttyPath( tty )
, m_ttyFd( UartInvalidValue )
, m_isOpened( false )
, m_isConfigured( false ) {}

Uart::~Uart() {
    deinit();
}

bool Uart::isInitialized() const {
    return m_isOpened && m_isConfigured;
}

int Uart::getInputBufferBitsCount( int& val ) {
    return ioctl( m_ttyFd, FIONREAD, &val );
}

int Uart::read( void* data, size_t size ) {
    if( isInitialized() ) {
        return ::read( m_ttyFd, data, size );
    }

    Log::error( cc::error( "Failed to read from UART: " ) + cc::p( m_ttyPath ) );
    return -1;
}

int Uart::write( const void* data, size_t size ) {
    if( isInitialized() ) {
        return ::write( m_ttyFd, reinterpret_cast< const char* >( data ), size );
    }

    Log::error( cc::error( "Failed to write to UART: " ) + cc::p( m_ttyPath ) );
    return -1;
}

bool Uart::isIoOk( int status ) {
    return status >= 0;
}

bool Uart::init() {
    if( isInitialized() ) {
        return true;
    }

    if( !( m_isOpened = open() ) ) {
        return false;
    }

    if( !( m_isConfigured = configure() ) ) {
        close();
        return false;
    }

    return true;
}

void Uart::deinit() {
    if( !isInitialized() )
        return;

    close();
}

bool Uart::open() {
    m_ttyFd = ::open( m_ttyPath.c_str(), O_RDWR | O_NOCTTY | O_CLOEXEC | O_NONBLOCK );
    if( m_ttyFd < 0 ) {
        Log::error( cc::error( "Open TTY failed, error: " ) + cc::note( std::to_string( errno ) ) );
        return false;
    }

    if( fcntl( m_ttyFd, F_SETFL, fcntl( m_ttyFd, F_GETFL ) & ~O_NONBLOCK ) < 0 ) {
        Log::error( cc::error( "Set TTY to block mode failed, error: " ) + cc::note( std::to_string( errno ) ) );
        closeAndClearFd();
        return false;
    }

    if( !isatty( m_ttyFd ) ) {
        Log::error( cc::error( "Not TTY" ) );
        closeAndClearFd();
        return false;
    }

    return true;
}

bool Uart::configure() {
    if( tcgetattr( m_ttyFd, &m_uartConfig ) < 0 ) {
        Log::error( cc::error( "Get TTY config failed" ) );
        closeAndClearFd();
        return false;
    }
    std::memcpy( &m_uartConfigBackup, &m_uartConfig,
                 sizeof( struct termios ) );  // backup current TTY config
    // 115200 8N1
    cfmakeraw( &m_uartConfig );
    if( cfsetspeed( &m_uartConfig, B115200 ) < 0 ) {
        Log::error( cc::error( "Set TTY baud rate failed" ) );
        closeAndClearFd();
        return false;
    }
    m_uartConfig.c_cflag &= ~CSIZE;
    m_uartConfig.c_cflag |= CS8;
    m_uartConfig.c_cflag &= ~( PARENB | PARODD );
    m_uartConfig.c_cflag |= 0;
    m_uartConfig.c_cflag &= ~CSTOPB;
    m_uartConfig.c_cflag |= 0;
    m_uartConfig.c_cflag &= ~CRTSCTS;
    m_uartConfig.c_cflag |= 0;
    m_uartConfig.c_cc[ VMIN ] = 1;
    m_uartConfig.c_cc[ VTIME ] = 0;

    return reset();
}

bool Uart::reset() {
    if( tcsetattr( m_ttyFd, TCSAFLUSH, &m_uartConfig ) < 0 ) {
        Log::error( cc::error( "Set TTY attrs failed" ) );
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

}  // namespace Zwave
}  // namespace HubPlatform
