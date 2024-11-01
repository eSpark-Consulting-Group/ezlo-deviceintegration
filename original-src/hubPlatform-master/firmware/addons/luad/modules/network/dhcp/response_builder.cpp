#include "luad/modules/network/dhcp/response_builder.h"
#include "json_utils/json_utils.h"
#include <netinet/in.h>

namespace HubPlatform {
namespace LuaNetwork {

namespace Dhcp {

#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/udp.h>

namespace Fields {
const char* ip = "ip";
const char* mask = "mask";
const char* gateway = "gateway";
const char* leaseTime = "lease_time";
const char* dhcp = "dhcp";
const char* domainName = "domain_name";
const char* dns = "dns";
const char* ntp = "ntp";

const char* hostname = "hostname";
const char* clientAddress = "client_address";
const char* destinationAddress = "destination_address";

const char* ipSource = "ip_source";
const char* ipDestination = "ip_destination";
const char* params = "params";
};  // namespace Fields

DhcpResponseBuilder::DhcpResponseBuilder( const ethhdr* eth, const iphdr* ip, const DhcpPacket& packet )
: m_eth( eth )
, m_ip( ip )
, m_packet( packet ) {}

std::string DhcpResponseBuilder::formatIpAddress( uint32_t addr ) {
    unsigned shift[ 4 ] = { 24, 16, 8, 0 };
    std::string result;
    result.reserve( 16 );

    char buf[ 4 ];
    for( unsigned i = 0; i < sizeof( shift ) / sizeof( shift[ 0 ] ); i++ ) {
        unsigned octet = ( addr >> shift[ i ] ) & 0xff;
        auto offset = sprintf( buf, "%u", octet );
        if( i < 3 )
            sprintf( buf + offset, "." );
        result += buf;
    }
    return result;
}

Json DhcpResponseBuilder::formatParams( const DhcpPacket& packet, const DhcpPacketData& data ) {
    Json params = Json::object();

    using namespace Dhcp;
    auto messageType = data.messageType();
    switch( messageType ) {
    case DHCPDISCOVERY: {
        params = formatDiscoveryParams( data, params );
    } break;
    case DHCPREQUEST: {
        params = formatRequestParams( data, params );

    } break;
    case DHCPOFFER:
    case DHCPACK: {
        params = formatOfferAndPack( packet, data, params );
    } break;
    default:;
    }

    return params;
}

Json& DhcpResponseBuilder::formatOfferAndPack( const DhcpPacket& packet, const DhcpPacketData& data, Json& params ) {
    auto offeredIp = ntohl( packet.yiaddr );
    auto mask = data.getNumber( SUBNET_MASK, 0x00000000 );
    const auto& gatewayArr = data.getIpArray( ROUTERS_IP_LIST );
    auto leaseTime = data.getNumber( LEASE_TIME );
    const auto& dhcpServer = data.getIpArray( DHCP_SERVER_IDENTIFIER );
    const auto& dnsArr = data.getIpArray( DNS_IP_LIST );
    auto domainName = data.getString( DOMAIN_NAME );
    const auto& ntpArr = data.getIpArray( NTP_IP_LIST );

    auto offeredIpStr = formatIpAddress( offeredIp );
    auto maskStr = formatIpAddress( mask );

    auto formatFirst = []( const DhcpPacketData::IpArray& arr ) {
        return arr.empty() ? "" : formatIpAddress( arr[ 0 ] );
    };
    auto formatAll = []( const DhcpPacketData::IpArray& arr ) {
        std::vector< std::string > results;
        results.reserve( arr.size() );
        std::for_each(
            begin( arr ), end( arr ), [&]( unsigned ip ) -> void { results.emplace_back( formatIpAddress( ip ) ); } );
        return results;
    };
    auto gatewayStr = formatFirst( gatewayArr );
    auto dhcpServerStr = formatFirst( dhcpServer );
    auto routers = formatAll( gatewayArr );

    json_add_str( params, Fields::ip, offeredIpStr );
    json_add_str( params, Fields::mask, maskStr );
    json_add_str( params, Fields::gateway, gatewayStr );
    json_add_long( params, Fields::leaseTime, leaseTime );

    json_add_str( params, Fields::dhcp, dhcpServerStr );
    json_add_str( params, Fields::domainName, domainName );

    // add dns array
    addIpArray( params, Fields::dns, dnsArr );
    // add ntp array
    if( !ntpArr.empty() )
        addIpArray( params, Fields::ntp, ntpArr );
    return params;
}

Json& DhcpResponseBuilder::formatRequestParams( const DhcpPacketData& data,
                                                Json& params ) {  // required: ip, gateway
    auto ip = data.getNumber( REQUESTED_IP_ADDRESS, 0 );
    json_add_str( params, "ip", formatIpAddress( ip ) );

    // gateway == routers[0]
    auto gatewayArr = data.getIpArray( ROUTERS_IP_LIST );
    std::string gateway;
    if( !gatewayArr.empty() ) {
        auto firstRouter = *gatewayArr.begin();
        gateway = formatIpAddress( firstRouter );
    }
    json_add_str( params, Fields::gateway, gateway );

    // optional: hostname, ntp
    auto hostName = data.clientHostName();
    if( !hostName.empty() )
        json_add_str( params, Fields::hostname, hostName );

    auto ntpArr = data.getIpArray( NTP_IP_LIST );
    if( !ntpArr.empty() ) {
        Json arr = Json::array();
        for( auto ntpIp : ntpArr ) {
            arr.push_back( formatIpAddress( ntpIp ) );
        }
        params[ Fields::ntp ] = arr;
    }
    return params;
}

Json& DhcpResponseBuilder::formatDiscoveryParams( const DhcpPacketData& data, Json& params ) {
    auto hostName = data.clientHostName();  // optional
    if( !hostName.empty() )
        json_add_str( params, Fields::hostname, hostName );
    return params;
}

void DhcpResponseBuilder::addIpArray( Json& params, const char* key, const DhcpPacketData::IpArray& ipArray ) {
    Json arr = Json::array();
    if( !key )
        key = "";
    for( auto ip : ipArray )
        arr.push_back( formatIpAddress( ip ) );
    params[ key ] = arr;
}

std::string hexAddress( const unsigned char* headerAddress, unsigned length, unsigned char delimiter ) {
    const unsigned maxLength = 6;
    if( length > maxLength )
        throw DhcpPacketError( "formatMac: length must be <= 6" );

    char buf[ maxLength * 3 + 2 ];  // 20
    if( headerAddress && length ) {
        for( unsigned i = 0, offset = 0; i < length; i++ ) {
            offset += sprintf( buf + offset, "%02x", headerAddress[ i ] );

            if( delimiter > 0 && i < length - 1 )
                offset += sprintf( buf + offset, "%c", delimiter );
        }
    }
    return std::string( buf );
}

/** Fields to format:
 *      - client_address: mac address,
 *      - destination_address: mac address,
 *      - ip_source: IP of DHCP server/last known client address
 *      - ip_destination: specific subnet broadcast address
 *
 * @param eth - ethernet header
 * @param ip - IP header
 * @return formatted json object
 */
Json DhcpResponseBuilder::formatHeader( const struct ethhdr* eth, const struct iphdr* ip ) {
    Json header = Json::object();

    std::string source, dest;
    if( eth ) {
        source = hexAddress( eth->h_source, 6, ':' );
        dest = hexAddress( eth->h_dest, 6, ':' );
    }
    // required fields
    json_add_str( header, Fields::clientAddress, source );
    json_add_str( header, Fields::destinationAddress, dest );

    source.clear();
    dest.clear();

    if( ip ) {
        source = formatIpAddress( ntohl( ip->saddr ) );
        dest = formatIpAddress( ntohl( ip->daddr ) );
    }
    // required fields
    json_add_str( header, Fields::ipSource, source );
    json_add_str( header, Fields::ipDestination, dest );
    return header;
}

Json DhcpResponseBuilder::buildResponse() {
    Json result = formatHeader( m_eth, m_ip );

    m_unpackedData = DhcpPacketUnpack::deserialize( m_packet );
    Json params = formatParams( m_packet, m_unpackedData );
    result[ Fields::params ] = params;
    return result;
}

Dhcp::ECommand DhcpResponseBuilder::messageType() const {
    return static_cast< Dhcp::ECommand >( m_unpackedData.messageType() );
}

}  // namespace Dhcp
}  // namespace LuaNetwork
}  // namespace HubPlatform
