#include "luad/modules/network/socket.h"

#include "common_utils/cpp14.h"
#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"
#include "json_utils/json_utils.h"

#include <cassert>
#include <netinet/ip.h> /* IPv4 */
#include <arpa/inet.h>
#include <netdb.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <cstring>
#include <fcntl.h>
#include <netinet/tcp.h>

namespace HubPlatform {
namespace LuaNetwork {

ISocket::ISocket()
: m_socket( -1 ) {}

bool ISocket::valid() const {
    return m_socket != -1;
}
int ISocket::getSocket() const {
    return m_socket;
}

class NetworkSocket final : public ISocket {
public:
    NetworkSocket();
    ~NetworkSocket() override;

    NetworkSocket( const NetworkSocket& rhs ) = delete;
    NetworkSocket& operator=( const NetworkSocket& rhs ) = delete;

    void createSocket( int domain, int type, int protocol ) override;
    int connect( const sockaddr* addr, socklen_t len ) override;
    int connect( const std::string& address, const std::string& port, int addressFamily = AF_INET ) override;
    int bind( const sockaddr* addr, socklen_t len ) override;
    virtual int setopt( int level, int name, int value ) override;
    virtual int setopt( int level, int name, const JsonObject& optionValue ) override;
    ssize_t send( const void* buf, size_t n, int flags ) override;
    ssize_t recv( void* buf, size_t n, int flags ) override;
    ssize_t sendto( const void* buf, size_t n, int flags, const sockaddr* addr, socklen_t addrLen ) override;
    ssize_t recvfrom( void* buf, size_t n, int flags, sockaddr* addr, socklen_t addrLen ) override;
    int getsockname( sockaddr* addr, socklen_t addrLen ) override;
    int close() override;
    int shutdown( unsigned flags ) override;
    int setTimeout( int socketLevel, int option, std::chrono::milliseconds timeoutMs ) override;
};

NetworkSocket::NetworkSocket() {
    m_socket = -1;
}

NetworkSocket::~NetworkSocket() {
    close();
}

void NetworkSocket::createSocket( int domain, int type, int protocol ) {
    if( m_socket != -1 ) {
        LOGGER_WARN( cc::warn( "Socket is not closed: " ) + cc::num0x( m_socket ) );
        close();
    }

    m_socket = socket( domain, type, protocol );

    LOGGER_TRACE( cc::debug( "Socket has created: " ) + cc::num0x( m_socket ) );
}

int NetworkSocket::connect( const sockaddr* addr, socklen_t len ) {
    return ::connect( m_socket, addr, len );
}

int NetworkSocket::connect( const std::string& address, const std::string& port, int addressFamily ) {
    sockaddr_in sa;

    std::memset( &sa, 0, sizeof( sa ) );

    sa.sin_family = addressFamily;
    inet_pton( addressFamily, address.c_str(), &sa.sin_addr );
    sa.sin_port = htons( std::stoi( port ) );

    return connect( reinterpret_cast< sockaddr* >( &sa ), sizeof( sa ) );
}

int NetworkSocket::bind( const sockaddr* addr, socklen_t len ) {
    int flags = fcntl( m_socket, F_GETFL, 0 );
    flags &= ( ~( O_NONBLOCK ) );
    fcntl( m_socket, F_SETFL, flags );

    int result = ::bind( m_socket, addr, len );

    if( result != 0 ) {
        LOGGER_WARN( cc::warn( "Cannot bind socket. Error code: " ) + cc::num10( errno ) );
    }

    flags = fcntl( m_socket, F_GETFL, 0 );
    flags |= O_NONBLOCK;
    fcntl( m_socket, F_SETFL, flags );

    return result;
}

int NetworkSocket::setopt( int level, int name, int value ) {
    return ::setsockopt( m_socket, level, name, reinterpret_cast< char* >( &value ), sizeof( value ) );
}

int NetworkSocket::setopt( int level, int name, const JsonObject& optionValue ) {
    auto multicastIp = json_get_str( optionValue, "multi_addr" );
    auto interface = json_get_str( optionValue, "interface" );
    struct ip_mreq imr;
    if( interface.empty() ) {
        imr.imr_interface.s_addr = htonl( INADDR_ANY );
    }
    else {
        imr.imr_interface.s_addr = inet_addr( interface.c_str() );
    }
    imr.imr_multiaddr.s_addr = inet_addr( multicastIp.c_str() );

    return ::setsockopt( m_socket, level, name, (char*) &imr, sizeof( imr ) );
}

int NetworkSocket::setTimeout( int socketLevel, int option, std::chrono::milliseconds timeoutMs ) {
    struct timeval time;
    time.tv_sec = ( timeoutMs.count() / 1000 );
    time.tv_usec = ( timeoutMs.count() % 1000 ) * 1000;

    return ::setsockopt( m_socket, socketLevel, option, &time, sizeof( time ) );
}

ssize_t NetworkSocket::send( const void* buf, size_t n, int flags ) {
    return ::send( m_socket, buf, n, flags );
}

ssize_t NetworkSocket::recv( void* buf, size_t n, int flags ) {
    return ::recv( m_socket, buf, n, flags );
}

ssize_t NetworkSocket::sendto( const void* buf, size_t n, int flags, const sockaddr* addr, socklen_t addrLen ) {
    return ::sendto( m_socket, buf, n, flags, addr, addrLen );
}

ssize_t NetworkSocket::recvfrom( void* buf, size_t n, int flags, sockaddr* addr, socklen_t addrLen ) {
    return ::recvfrom( m_socket, buf, n, flags, addr, &addrLen );
}

int NetworkSocket::getsockname( sockaddr* addr, socklen_t addrLen ) {
    return ::getsockname( m_socket, addr, &addrLen );
}

int NetworkSocket::close() {
    if( m_socket != -1 ) {
        auto ret = ::close( m_socket );

        LOGGER_TRACE( cc::trace( "Socket has closed: " ) + cc::num0x( m_socket ) + cc::trace( ". Code: " ) +
                      cc::num10( ret ) );

        m_socket = -1;

        return ret;
    }

    return -1;
}

int NetworkSocket::shutdown( unsigned flags ) {
    auto ret = ::shutdown( m_socket, flags );
    return ret;
}

std::unique_ptr< ISocket > makeNetworkSocket() {
    return std::make_unique< NetworkSocket >();
}

}  // namespace LuaNetwork
}  // namespace HubPlatform
