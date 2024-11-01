#include "luad/modules/network/dhcp/dhcp_listener.h"
#include "luad/modules/network/dhcp/dhcp.h"
#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"
#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstring>
#include <utility>

namespace HubPlatform {
namespace LuaNetwork {

namespace Dhcp {

#include <net/if.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/if_packet.h>

DhcpListener::DhcpListener( ResultCallback callback )
: m_isRunning( false )
, m_callback( std::move( callback ) ) {}

DhcpListener::~DhcpListener() {
    DhcpListener::stop();
}

void DhcpListener::setSocket( std::unique_ptr< ISocket >&& socket ) {
    m_socket = std::move( socket );
}

void DhcpListener::setFilter( FilterCallback&& filter ) {
    m_filterCallback = std::move( filter );
}

void DhcpListener::scan() {
    // run listening thread here
    if( !m_isRunning ) {
        m_thread = std::thread( &DhcpListener::run, this );
        m_isRunning = true;
    }
}

void DhcpListener::run() {
    try {
        runThrowErrors();
    }
    catch( const DhcpSocketError& e ) {
        Log::error( std::string{ "DhcpListener::run: continue. Exception: " } + e.what() );
    }
}

NetworkInterfaces::InterfaceNames listInterfaceNames();

void DhcpListener::runThrowErrors() {
    if( !m_socket )
        m_socket = makeNetworkSocket();
    if( !m_socket ) {
        throw DhcpSocketError( "Could not create socket" );
    }

    // configure socket
    m_socket->createSocket( AF_PACKET, SOCK_RAW, htons( ETH_P_ALL ) );
    if( m_socket->getSocket() == -1 ) {
        Log::error( std::string{ "Could not open raw socket: errno: " } + strerror( errno ) );
        throw DhcpSocketError( std::string{ "Could not open raw socket, errno: " } + strerror( errno ) );
    }

    // allocate buffers
    const unsigned MAX_BUF_LEN = 65536;
    unsigned bufLen = MAX_BUF_LEN;

    std::vector< uint8_t > buffer( MAX_BUF_LEN, 0 );
    memset( buffer.data(), 0, bufLen );
    sockaddr addr = {};
    int addrLen = sizeof( addr );

    while( true ) {
        // listen socket while pipe is not closed
        // Receive a network packet and copy in to buffer
        ssize_t receivedLen = m_socket->recvfrom( buffer.data(), MAX_BUF_LEN, 0, &addr, addrLen );
        if( receivedLen < 0 ) {
            Log::error( cc::error( "Error in reading recvfrom function: " ) + strerror( errno ) );
            return;
        }

        try {
            Result headers = unpackHeaders( buffer.data(), receivedLen );

            bool okFilter = filterPacket( headers );
            if( okFilter ) {
                notify( headers );
            }
        }
        catch( const DhcpPacketError& e ) {
            // invalid packet, not an UDP packet, skip
            continue;
        }
        catch( const std::exception& ex ) {
            LOGGER_ERROR( cc::error( "Exception: " ) + cc::note( ex.what() ) + cc::error( ", exception type: " ) +
                          cc::note( typeid( ex ).name() ) );
        }
    }
}

DhcpListener::Result DhcpListener::unpackHeaders( uint8_t* buffer, unsigned length ) {
    DhcpPacketUnpack unpacker;

    auto headers = unpacker.networkPacketUnpack( buffer, length );
    if( headers == nullptr )
        throw DhcpPacketError( "Could not fetch DHCP headers!" );
    return headers;
}

/** Filter by dhcp port */
bool defaultFilter( const udphdr& udp ) {
    const unsigned filterPort = DHCP_SERVER_PORT;
    bool okFilter = ( filterPort > 0 && ( ntohs( udp.dest ) == filterPort || ntohs( udp.source ) == filterPort ) );
    return okFilter;
}

bool DhcpListener::filterPacket( const Result& headersPtr ) {
    if( !headersPtr )
        return false;

    udphdr udp = {};
    std::tie( std::ignore, std::ignore, udp, std::ignore ) = *headersPtr;
    // filter by dhcp port
    bool portFilterOk = defaultFilter( udp );
    if( portFilterOk && m_filterCallback )
        return m_filterCallback( headersPtr );
    return portFilterOk;
}

void DhcpListener::notify( Result& headersPtr ) {
    if( m_callback == nullptr )
        return;
    m_callback( std::move( headersPtr ) );
}

void DhcpListener::stop() {
    if( m_socket && m_socket->getSocket() > 0 ) {
        // shutdown socket before closing
        m_socket->shutdown( SHUT_RDWR );  // to exit from blocking recvfrom
        m_socket->close();
    }
    if( m_thread.joinable() )
        m_thread.join();
    m_isRunning = false;
}

bool DhcpListener::isScanning() const {
    return m_isRunning;
}

NetworkInterfaces::InterfaceNames listInterfaceNames() {
    NetworkInterfaces allInterfaces;
    return allInterfaces.getInterfaceNames();
}

std::unique_ptr< INetworkScan > makeListener( DhcpListener::ResultCallback&& callback ) {
    return std::make_unique< DhcpListener >( std::move( callback ) );
}

DhcpSocketError::DhcpSocketError( const std::string& err ) noexcept
: std::runtime_error( std::string{ "DhcpSocketError: " }.append( err ) ) {}

}  // namespace Dhcp
}  // namespace LuaNetwork
}  // namespace HubPlatform