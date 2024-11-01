#pragma once

#include <string>
#include <vector>
#include <array>
#include <cstdint>
#include <set>
#include <memory>

#include <sys/socket.h>

namespace HubPlatform {
namespace Network {
namespace Utils {

struct InterfaceInfo {
    enum class Type { IPV4 = 0, IPV6 };
    struct Address {
        Type m_type;
        sockaddr m_ipAddress;
        sockaddr m_mask;
    };
    using MacAddress = std::array< uint8_t, 6 >;

    std::string m_name;
    std::vector< Address > m_addressList;
    MacAddress m_macAddress;
    unsigned int m_ifaFlags = 0;  // SIOCGIFFLAGS

    bool isUp();
};

std::string sockadrToString( const sockaddr& sa );
std::string macAddrToString( const InterfaceInfo::MacAddress& address );
std::vector< InterfaceInfo > getSystemInterfaces( const std::set< std::string >& names = {} );
std::string purgeCRSymbol( const std::string& atCommand );

std::string getDefaultGatewayIPv4( const std::string& interface );

struct NetstatRecord {
    std::string proto;
    std::string localAddress;
    std::string foreignAddress;
    std::string state;
    std::string pid;
    std::string program;
};
std::vector< NetstatRecord > getNetstatRecords();

void restartNetworkService();
void logWirelessConfig();

}  // namespace Utils
}  // namespace Network
}  // namespace HubPlatform
