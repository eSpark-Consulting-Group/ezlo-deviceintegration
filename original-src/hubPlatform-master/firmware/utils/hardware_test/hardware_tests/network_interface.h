#pragma once

#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <vector>

namespace HubPlatform {
namespace HardwareTest {

class NetworkInterface {
    static constexpr uint32_t LoopbackIp = INADDR_LOOPBACK;

public:
    using NetworkInterfaceUPtr = std::unique_ptr< NetworkInterface >;
    static NetworkInterfaceUPtr createInterface( struct ifaddrs* pInterface );
    static bool isWired( const std::string& ifName );
    static bool isWireless( const std::string& ifName );
    static std::vector< std::string > getSupportedInterfaces();
    static void setInterfaceUp( const std::string& interfaceName, const bool up = true );
    static bool isUp( const std::string& interfaceName );

public:
    NetworkInterface() = delete;
    ~NetworkInterface() = default;

private:
    explicit NetworkInterface( struct ifaddrs* pInterface );

public:
    const std::string& getName() const;
    std::string getAddress() const;
    bool hasAddress() const;
    bool isUp() const;
    bool isLoopback() const;
    bool isWired() const;
    bool isWireless() const;
    bool doPingTest( const std::string& remoteResource = "8.8.8.8" );

private:
    const std::string m_name;
    struct sockaddr_in m_socketAddress;
    char m_ipAddressStr[ INET_ADDRSTRLEN ];
    // describes initial state
    const unsigned m_ifFlags;
};

}  // namespace HardwareTest
}  // namespace HubPlatform
