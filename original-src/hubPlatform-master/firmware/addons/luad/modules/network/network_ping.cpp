#include "network_ping.h"
#include "log_utils/ezlo_log.h"
#include <sys/time.h>
#include <tiny/tiny_consolecolors.h>

namespace HubPlatform {
namespace LuaNetwork {

NetworkPing::NetworkPing( const IAddress& address )
: m_socket( socket( AF_INET, SOCK_RAW, IPPROTO_ICMP ) )
, m_address( address ) {}

NetworkPing::~NetworkPing() {
    ::close( m_socket );
}

NetworkPing::Result NetworkPing::sendPing() {
    if( m_socket < 0 ) {
        return Result::SOCKET_NOT_OPENED;
    }

    if( !m_isInitialized ) {
        auto initResult = initPing();
        if( initResult != Result::OK )
            return initResult;
    }

    const uint32_t pingPacketSize = 64;
    struct PingPacket {
        struct icmphdr hdr;
        char msg[ pingPacketSize - sizeof( struct icmphdr ) ];
    };

    PingPacket pingPacket;
    bzero( &pingPacket, sizeof( pingPacket ) );
    pingPacket.hdr.type = ICMP_ECHO;
    pingPacket.hdr.un.echo.id = getpid();
    int msgSize = sizeof( pingPacket.msg ) - 1;
    for( int i = 0; i < msgSize; i++ ) {
        pingPacket.msg[ i ] = static_cast< char >( i + '0' );
    }
    pingPacket.msg[ msgSize ] = 0;
    pingPacket.hdr.un.echo.sequence = m_sequenceCounter++;
    pingPacket.hdr.checksum = checksum( &pingPacket, sizeof( pingPacket ) );

    auto address = m_address.getSocketAddress();
    if( sendto( m_socket, &pingPacket, sizeof( pingPacket ), 0, &address.address, address.size ) <= 0 ) {
        return Result::SEND_FAILED;
    }

    struct sockaddr receiveAddress = {};
    uint32_t addressLength = sizeof( receiveAddress );
    if( recvfrom( m_socket, &pingPacket, sizeof( pingPacket ), 0, &receiveAddress, &addressLength ) <= 0 ) {
        return Result::RECEIVE_FAILED;
    }
    if( !( pingPacket.hdr.type == 69 && pingPacket.hdr.code == 0 ) ) {
        return Result::PING_ERROR;
    }
    return Result::OK;
}

unsigned short NetworkPing::checksum( void* b, int len ) {
    unsigned short* buf = static_cast< unsigned short* >( b );
    unsigned int sum = 0;
    unsigned short result;

    for( sum = 0; len > 1; len -= 2 )
        sum += *buf++;
    if( len == 1 )
        sum += *(unsigned char*) buf;
    sum = ( sum >> 16 ) + ( sum & 0xFFFF );
    sum += ( sum >> 16 );
    result = ~sum;
    return result;
}

NetworkPing::Result NetworkPing::initPing() {
    if( m_socket < 0 ) {
        return Result::SOCKET_NOT_OPENED;
    }
    // set socket options at ip to TTL and value to 64,
    int ttl_val = 64;
    if( setsockopt( m_socket, SOL_IP, IP_TTL, &ttl_val, sizeof( ttl_val ) ) != 0 ) {
        return Result::SOCKET_SET_OPTION_FAILED;
    }
    // setting timeout of recv setting
    timeval tv_out = { 1, 0 };
    if( setsockopt( m_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*) &tv_out, sizeof tv_out ) != 0 ) {
        return Result::SOCKET_SET_OPTION_FAILED;
    }
    m_isInitialized = true;
    return Result::OK;
}

int NetworkPing::getSequenceCounter() const {
    return m_sequenceCounter;
}

}  // namespace LuaNetwork
}  // namespace HubPlatform
