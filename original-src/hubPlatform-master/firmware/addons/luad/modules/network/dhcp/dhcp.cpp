#include "log_utils/ezlo_log.h"
#include "dhcp.h"
#include <cstring>
#include <arpa/inet.h>

namespace HubPlatform {
namespace LuaNetwork {
namespace Dhcp {

#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/udp.h>

bool DhcpPacket::checkMagic() const {
    const uint8_t dhcpMagicNumber[] = { 99, 130, 83, 99 };
    bool goodMagic = 0 == memcmp( this->magicCookie, dhcpMagicNumber, sizeof( dhcpMagicNumber ) );
    bool goodOp = this->op == BOOTREQUEST || this->op == BOOTREPLY;

    return goodMagic && goodOp;
}

void DhcpPacket::clear() {
    memset( this, 0, sizeof( DhcpPacket ) );
}

void DhcpPacket::assign( unsigned char* data, unsigned int length ) {
    auto maxSize = length > sizeof( DhcpPacket ) ? sizeof( DhcpPacket ) : length;
    memcpy( this, data, maxSize );
}

DhcpPacket::DhcpPacket() {
    clear();
}

DhcpPacketData& DhcpPacketData::setString( unsigned option, std::string&& value ) {
    m_stringOptions.emplace( option, std::move( value ) );
    return *this;
}

DhcpPacketData& DhcpPacketData::setString( unsigned option, const std::string& value ) {
    m_stringOptions[ option ] = value;
    return *this;
}

std::string DhcpPacketData::getString( unsigned option ) const {
    auto found = m_stringOptions.find( option );
    return ( found != m_stringOptions.end() ) ? found->second : std::string();
}

std::string DhcpPacketData::clientHostName() const {
    return getString( Dhcp::HOST_NAME );
}

DhcpPacketData& DhcpPacketData::setNumber( unsigned option, unsigned value ) {
    m_numberOptions[ option ] = value;
    return *this;
}

unsigned DhcpPacketData::getNumber( unsigned option, unsigned defaultValue ) const {
    auto found = m_numberOptions.find( option );
    return ( found != m_numberOptions.end() ) ? found->second : defaultValue;
}

DhcpPacketData& DhcpPacketData::setIpArray( unsigned option, const IpArray& ips ) {
    m_ipArrayMap[ option ] = ips;
    return *this;
}

DhcpPacketData& DhcpPacketData::setIpArray( unsigned option, IpArray&& ips ) {
    m_ipArrayMap.emplace( option, ips );
    return *this;
}

/** Searches IP addresses array corresponding to some option.
 * @param option - id of a option
 * @return found array, or empty array, if nothing was found
 */
const DhcpPacketData::IpArray& DhcpPacketData::getIpArray( unsigned option ) const {
    auto found = m_ipArrayMap.find( option );
    return ( found != m_ipArrayMap.end() ) ? found->second : m_emptyIpArraySearchResult;
}

unsigned DhcpPacketData::messageType() const {
    return getNumber( Dhcp::MESSAGE_TYPE );
}

bool DhcpPacketUnpack::setBuffer( const unsigned char* pBuffer, unsigned int length ) {
    if( pBuffer == nullptr || length == 0 )
        return false;

    m_buffer.reserve( length );
    m_buffer.assign( pBuffer, pBuffer + length );
    return true;
}

DhcpPacket DhcpPacketUnpack::unpack() {
    DhcpPacket packet;
    if( empty() )
        return packet;

    packet.assign( m_buffer.data(), m_buffer.size() );

    if( !packet.checkMagic() )
        throw DhcpPacketError( "DHCP Unpack Error: invalid operation" );
    return packet;
}

/** Unpacks ethernet/ip/udp packet and gets DHCP payload part. returns all listed as a pointer to tuple.
 * Throw DhcpPacketError on any parsing problems */
DhcpPacketUnpack::AllHeadersPtr DhcpPacketUnpack::networkPacketUnpack( const unsigned char* buffer, unsigned length ) {
    if( buffer == nullptr || 0 == length )
        throw DhcpPacketError( "networkPacketUnpack: invalid packet buffer" );

    struct ethhdr* eth = (struct ethhdr*) buffer;
    struct iphdr* ip = (struct iphdr*) ( buffer + sizeof( struct ethhdr ) );
    unsigned short ipHeaderLen = ip->ihl * 4;

    if( ip->protocol != IPPROTO_UDP )
        throw DhcpPacketError( "networkPacketUnpack: invalid packet protocol, UDP expected" );

    struct udphdr* udp = (struct udphdr*) ( buffer + sizeof( struct ethhdr ) + ipHeaderLen );
    int payloadLength = length - ( ipHeaderLen + sizeof( struct ethhdr ) + sizeof( struct udphdr ) );
    const uint8_t* payload = ( buffer + sizeof( struct ethhdr ) + ipHeaderLen + sizeof( struct udphdr ) );

    DhcpPacketUnpack unpacker;
    unpacker.setBuffer( payload, payloadLength );

    DhcpPacket packet = unpacker.unpack();
    AllHeadersPtr ptr( new AllHeaders( *eth, *ip, *udp, packet ) );
    return ptr;
}

void logOptionData( unsigned tag, const uint8_t* data, unsigned length ) {
    if( !data || !length )
        return;
    char dataDump[ 32 ] = { 0 };
    uint32_t word;

    switch( length ) {
    case 1:
        word = *data;
        break;
    case 2:
        word = ntohs( *(uint16_t*) data );
        break;
    case 4:
        word = ntohl( *(uint32_t*) data );
        break;
    default:;
    }
    if( length <= 4 )
        snprintf( dataDump, sizeof( dataDump ), "0x%02x (%u)", word, word );
    Log::print(
        Log::DEBUG, "Dhcp Option: tag: %u, length: %u, data: %s\n", (unsigned) tag, (unsigned) length, dataDump );
}

DhcpPacketData DhcpPacketUnpack::deserialize( const DhcpPacket& packet ) {
    if( !packet.checkMagic() ) {
        throw DhcpPacketError( "Deserialize dhcp packet: dhcp::magicCookie doesn't match" );
    }

    const uint8_t* options = packet.options;
    const uint8_t* current = options;
    const unsigned option0 = 0, option255 = 255;
    const unsigned tagOffset = 0, lengthOffset = 1, dataOffset = 2;

    DhcpPacketData dest;

    int i = 0;
    for( ; i < 5000; i++ ) {
        uint8_t tag = current[ tagOffset ];
        uint8_t length = 1;
        if( 0 == i ) {
            // tag should be equal to 0
            if( 0 != tag ) {
                // invalid tag
            }
        }
        if( tag == option0 || tag == option255 ) {
            // 0, 255 - fixed.length 1-byte options
            if( option255 == tag ) {
                break;
            }
            current += 1;
        }
        else {
            length = current[ lengthOffset ];
            const uint8_t* data = current + dataOffset;

            using namespace Dhcp;

#ifdef _DEBUG
            logOptionData( tag, data, length );
#endif
            // All multi-octet quantities are in network byte-order.
            switch( tag ) {
            // request type: length = 1
            case MESSAGE_TYPE:  // 53
                dest.setNumber( tag, *data );
                break;
            case HOST_NAME:           // 12, client host name, var, 61, 60
            case NACK_ERROR_MESSAGE:  // NACK error message
                dest.setString( tag, std::string( data, data + length ) );
                break;
                // length: 4, IP addresses
            case SUBNET_MASK:             // 1: subnet mask
            case TIME_OFFSET:             // 2: time offset
            case REQUESTED_IP_ADDRESS:    // 50: requested IP address
            case LEASE_TIME:              // 51: IP address lease time
            case DHCP_SERVER_IDENTIFIER:  // 54: Server Identifier
                dest.setNumber( tag, ntohl( *(const uint32_t*) data ) );
                break;

                // length = 4 * N, N >= 1, array of IPv4 addresses
            case ROUTERS_IP_LIST:  // 3
            case NTP_IP_LIST:      // 4
            case DNS_IP_LIST:      // 5
                dest.setIpArray( tag, makeIpArray( data, length ) );
                break;
            default:
                // not found
                ;
            };

            if( current[ 0 ] == 0 ) {  // padding
                current += 1;
            }
            else {
                current += 2 + current[ lengthOffset ];
            }
        }
    }

    if( i == 5000 ) {
        // throw runtime error, recursion, possible over-read
        LOGGER_ERROR( "Possible bad-formed dhcp options, options reached index " + std::to_string( i ) );
    }
    return dest;
}

DhcpPacketData::IpArray DhcpPacketUnpack::makeIpArray( const uint8_t* data, unsigned length ) {
    DhcpPacketData::IpArray ips;
    ips.reserve( length / 4 );
    for( unsigned i = 0; i < length / 4; i++ ) {
        volatile auto ptr = reinterpret_cast< const uint32_t* >( &data[ i * 4 ] );
        uint32_t ip = ntohl( *ptr );
        ips.push_back( ip );
    }
    return ips;
}

}  // namespace Dhcp
}  // namespace LuaNetwork
}  // namespace HubPlatform