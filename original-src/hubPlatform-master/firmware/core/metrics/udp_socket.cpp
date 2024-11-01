#include "udp_socket.h"
#include <log_utils/ezlo_log.h>
#include <stdexcept>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <tiny/tiny_consolecolors.h>

namespace HubPlatform {
namespace Metrics {

class SocketCreationException : public std::runtime_error {
public:
    explicit SocketCreationException( const std::string& message )
    : std::runtime_error( message ) {}
};

UdpSocket::UdpSocket()
: m_socket( -1 ) {
    m_socket = socket( AF_INET, SOCK_DGRAM, 0 );
    if( m_socket < 0 ) {
        throw SocketCreationException( "Datagram socket creation failed!" );
    }
}

UdpSocket::~UdpSocket() {
    if( m_socket >= 0 ) {
        if( close( m_socket ) == -1 ) {
            Log::error( "Failed to close datagram socket" );
        }
    }
}

bool UdpSocket::sendTo( const void* data, size_t dataLen, const struct sockaddr* addr, socklen_t addrLen ) const {
    int result = sendto( m_socket, data, dataLen, 0, addr, addrLen );
    if( result == -1 ) {
        Log::error( "Failed to send UDP report, size: " + cc::num10( dataLen ) );
        return false;
    }
    return true;
}

sockaddr_in UdpSocket::getServerAddr( const std::string& serverUrl, size_t port ) {
    sockaddr_in servAddr = { 0 };
    memset( &servAddr, 0, sizeof( servAddr ) );

    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons( port );
    servAddr.sin_addr.s_addr = inet_addr( serverUrl.c_str() );

    return servAddr;
}

bool sendUdpReport( const void* data, size_t dataLen, const sockaddr_in& servAddr ) {
    try {
        UdpSocket socket;
        if( !socket.sendTo(
                data, dataLen, reinterpret_cast< const struct sockaddr* >( &servAddr ), sizeof( servAddr ) ) ) {
            return false;
        }
    }
    catch( const SocketCreationException& e ) {
        Log::error( "Udp socket creation failed: " + std::string( e.what() ) );
        return false;
    }
    catch( const std::exception& e ) {
        Log::error( "Udp socket operation failed: " + std::string( e.what() ) );
        return false;
    }
    return true;
}

}  // namespace Metrics
}  // namespace HubPlatform