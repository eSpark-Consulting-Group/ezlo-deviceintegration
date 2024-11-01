#pragma once
#include "luad/modules/network/dhcp/dhcp.h"
#include "json_utils/json_fwd.h"

namespace HubPlatform {
namespace LuaNetwork {

namespace Dhcp {

class DhcpResponseBuilder {
public:
    DhcpResponseBuilder( const ethhdr* eth, const struct iphdr* ip, const DhcpPacket& packet );

    static std::string formatIpAddress( uint32_t addr );

    static Json formatParams( const DhcpPacket& packet, const DhcpPacketData& data );
    static void addIpArray( Json& params, const char* key, const DhcpPacketData::IpArray& ipArray );

    static Json formatHeader( const struct ethhdr* eth, const struct iphdr* ip );

    Json buildResponse();

    Dhcp::ECommand messageType() const;

private:
    static Json& formatDiscoveryParams( const DhcpPacketData& data, Json& params );
    static Json& formatRequestParams( const DhcpPacketData& data, Json& params );
    static Json& formatOfferAndPack( const DhcpPacket& packet, const DhcpPacketData& data, Json& params );

private:
    const struct ethhdr* m_eth;
    const struct iphdr* m_ip;
    const DhcpPacket& m_packet;
    DhcpPacketData m_unpackedData;
};

namespace Fields {
extern const char* ip;
extern const char* mask;
extern const char* gateway;
extern const char* leaseTime;
extern const char* dhcp;
extern const char* domainName;
extern const char* dns;
extern const char* ntp;

extern const char* hostname;
extern const char* clientAddress;
extern const char* destinationAddress;

extern const char* ipSource;
extern const char* ipDestination;
extern const char* params;
};  // namespace Fields

}  // namespace Dhcp

}  // namespace LuaNetwork
}  // namespace HubPlatform