#pragma once

#include <cstdint>
#include <vector>
#include <map>
#include <string>
#include <memory>

namespace HubPlatform {
namespace LuaNetwork {

namespace Dhcp {

// Avoid conflicts with network_manager's netinet/ip_icmp.h
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/udp.h>

enum ERequestType {
    BOOTREQUEST = 1,
    BOOTREPLY = 2,
};
enum ECommand {
    DHCPDISCOVERY = 1,
    DHCPOFFER = 2,
    DHCPREQUEST = 3,
    DHCPDECLINE = 4,
    DHCPACK = 5,
    DHCPNACK = 6,
    DHCPRELEASE = 7,
    DHCPINFORMATION = 8
};
enum EOptions {
    MESSAGE_TYPE = 53,
    HOST_NAME = 12,
    DOMAIN_NAME = 15,
    BROADCAST_IP_ADDRESS = 28,
    REQUESTED_IP_ADDRESS = 50,
    LEASE_TIME = 51,
    SUBNET_MASK = 1,
    TIME_OFFSET = 2,
    ROUTERS_IP_LIST = 3,  // GATEWAY = ROUTERS_IP_LIST[0]
    NTP_IP_LIST = 4,
    DNS_IP_LIST = 6,
    DHCP_SERVER_IDENTIFIER = 54,
    NACK_ERROR_MESSAGE = 56,
    RENEWAL_TIME = 58,
    REBINDING_TIME = 59,
};

enum EPort { DHCP_SERVER_PORT = 67, DHCP_CLIENT_PORT = 68 };

class DhcpPacketUnpack;

/** DHCP packet deserialized,
 * according to https://tools.ietf.org/html/rfc2131#page-8
 */
struct DhcpPacket {
    enum ELimits { MAX_CHADDR_LENGTH = 16, MAX_SNAME_LENGTH = 64, MAX_FILE_LENGTH = 128, MAX_OPTIONS_LENGTH = 312 };

    enum EEthernet { ETHERNET_HARDWARE_ADDRESS = 1, ETHERNET_HARDWARE_ADDRESS_LENGTH = 6 };

    /** Options for results:
     *
     *  Common fields
     *      client_address - from ethernet header, eth->h_source
     *      destination_address - from ethernet header, eth->h_dest
     *      ip_source - from ip header, ip.source, see iphdr::saddr
     *      ip_destination - from ip header, ip.dest, see iphdr::daddr
     *
     *  Discover:
     *      params:
     *          + hostname - option[12]
     *
     *  Request
     *      params:
     *          + ip - options[50], request IP address
     *          + hostname - options[12], optional
     *          - gateway - string, but actually, ip[], router, options[3]
     *          + npt - ip[], options[4], optional
     *
     *  Offer/Acknowledge
     *      params:
     *          + ip - dhcp header, yiaddr, Offered IP
     *          + mask - options[1], SUBNET_MASK
     *          + gateway - Router, options[3], ip[]
     *          + lease_time - tag_ip_lease, 51
     *          + dhcp - options[54], DHCP Server Identifier, ip address
     *          + dns - ip[], options[5]
     *          + domain_name - string, options[15], client should use this domain name to resolve names via DNS
     *          + ntp - ip[], options[4] (???  options[42])


     * Note: The first four octets of the 'options' field of the DHCP message
   contain the (decimal) values 99, 130, 83 and 99, respectively (this
   is the same magic cookie as is defined in RFC 1497 [17]).
     */

public:
    // All fields in network order, convert with ntohs/ntohl to intel/amd before using them
    uint8_t op;                             // BOOTREQUEST: 1, BOOTREPLY: 2
    uint8_t htype;                          // hardware address type
    uint8_t hlen;                           // Hardware address length
    uint8_t hops;                           // client: 0,
    uint32_t xid;                           // transaction ID
    uint16_t secs;                          // filled by client, seconds elapsed
    uint16_t flags;                         // flags
    uint32_t ciaddr;                        // client IP address, client can respond on ARP requests
    uint32_t yiaddr;                        // client's IP address (Your-address)
    uint32_t siaddr;                        // next server to use
    uint32_t giaddr;                        // relay agent IP addrss
    uint8_t chaddr[ MAX_CHADDR_LENGTH ];    // client hardware address
    uint8_t sname[ MAX_SNAME_LENGTH ];      // optional: server host name, null-terminated
    uint8_t file[ MAX_FILE_LENGTH ];        // boot file name
    uint8_t magicCookie[ 4 ];               // magic cookie, 'dhcp': { 99, 130, 83, 99 }, hex: 63 82 53 63
    uint8_t options[ MAX_OPTIONS_LENGTH ];  // options

public:
    DhcpPacket();

    bool checkMagic() const;
    void clear();
    void assign( unsigned char* data, unsigned length );
};

class DhcpPacketData {
public:
    DhcpPacketData() = default;
    DhcpPacketData& setString( unsigned option, const std::string& value );
    DhcpPacketData& setString( unsigned option, std::string&& value );
    std::string getString( unsigned option ) const;

    DhcpPacketData& setNumber( unsigned option, unsigned value );
    unsigned getNumber( unsigned option, unsigned defaultValue = 0 ) const;

    using IpArray = std::vector< unsigned >;
    DhcpPacketData& setIpArray( unsigned option, const IpArray& ips );
    DhcpPacketData& setIpArray( unsigned option, IpArray&& ips );
    const IpArray& getIpArray( unsigned option ) const;

    std::string clientHostName() const;
    unsigned messageType() const;

private:
    using StringMap = std::map< unsigned, std::string >;
    StringMap m_stringOptions;

    using NumMap = std::map< unsigned, unsigned >;
    NumMap m_numberOptions;

    using IpArrayMap = std::map< unsigned, IpArray >;
    IpArrayMap m_ipArrayMap;
    IpArray m_emptyIpArraySearchResult;
};

class DhcpPacketError : public std::runtime_error {
public:
    DhcpPacketError( const std::string& msg )
    : std::runtime_error( "DhcpPacketError: " + msg ) {}
};

class DhcpPacketUnpack {
public:
    bool setBuffer( const unsigned char* pBuffer, unsigned length );

    DhcpPacket unpack();

    using AllHeaders = std::tuple< ethhdr, iphdr, udphdr, DhcpPacket >;
    using AllHeadersPtr = std::unique_ptr< AllHeaders >;
    static AllHeadersPtr networkPacketUnpack( const unsigned char* buffer, unsigned length );

    bool empty() const {
        return m_buffer.empty();
    }

    static DhcpPacketData deserialize( const DhcpPacket& );
    static DhcpPacketData::IpArray makeIpArray( const uint8_t* data, unsigned length );

private:
    std::vector< uint8_t > m_buffer;
};

}  // namespace Dhcp

}  // namespace LuaNetwork

}  // namespace HubPlatform