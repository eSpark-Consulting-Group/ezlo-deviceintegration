#include "network_interface.h"

namespace HubPlatform {
namespace HardwareTest {

NetworkInterface::NetworkInterfaceUPtr NetworkInterface::createInterface( struct ifaddrs* pInterface ) {
    return ( pInterface == nullptr ? NetworkInterfaceUPtr() :
                                     NetworkInterfaceUPtr( new NetworkInterface( pInterface ) ) );
}

bool NetworkInterface::isWired( const std::string& ifName ) {
    return ( ifName.find( "eth" ) == 0 || ifName.find( "enp" ) == 0 );
}

bool NetworkInterface::isWireless( const std::string& ifName ) {
    return ( ifName.find( "wlan" ) == 0 || ifName.find( "ra" ) == 0 || ifName.find( "apcli" ) == 0 );
}

std::vector< std::string > NetworkInterface::getSupportedInterfaces() {
    // list interfaces
    struct if_nameindex* pInterfaceNameIndex;
    pInterfaceNameIndex = if_nameindex();
    std::vector< std::string > interfaces;
    if( pInterfaceNameIndex != NULL ) {
        for( struct if_nameindex* pInterface = pInterfaceNameIndex;
             pInterface->if_index != 0 || pInterface->if_name != NULL;
             ++pInterface ) {
            interfaces.push_back( pInterface->if_name );
        }
    }
    return interfaces;
}

void NetworkInterface::setInterfaceUp( const std::string& interfaceName, const bool up ) {
    int sockfd;
    sockfd = socket( AF_INET, SOCK_DGRAM, 0 );
    if( sockfd < 0 ) {
        std::cerr << __PRETTY_FUNCTION__ << " failed to open socket !" << std::endl;
        return;
    }

    struct ifreq ifr;
    memset( &ifr, 0, sizeof ifr );
    strncpy( ifr.ifr_name, interfaceName.c_str(), IFNAMSIZ );

    auto result = ioctl( sockfd, SIOCGIFFLAGS, &ifr );
    if( result == -1 ) {
        std::cerr << __PRETTY_FUNCTION__ << " failed to get interface flags !" << std::endl;
        return;
    }

    if( up ) {
        ifr.ifr_flags |= IFF_UP;
    }
    else {
        ifr.ifr_flags &= ~IFF_UP;
    }

    result = ioctl( sockfd, SIOCSIFFLAGS, &ifr );
    if( result == -1 ) {
        std::cerr << __PRETTY_FUNCTION__ << " failed to set interface flags !" << std::endl;
        return;
    }
}

bool NetworkInterface::isUp( const std::string& interfaceName ) {
    int sockfd;
    sockfd = socket( AF_INET, SOCK_DGRAM, 0 );
    if( sockfd < 0 ) {
        std::cerr << __PRETTY_FUNCTION__ << " failed to open socket !" << std::endl;
        return false;
    }

    struct ifreq ifr;
    memset( &ifr, 0, sizeof ifr );
    strncpy( ifr.ifr_name, interfaceName.c_str(), IFNAMSIZ );

    auto result = ioctl( sockfd, SIOCGIFFLAGS, &ifr );
    if( result == -1 ) {
        std::cerr << __PRETTY_FUNCTION__ << " failed to get interface flags !" << std::endl;
        return false;
    }

    return ( ifr.ifr_flags & IFF_UP );
}

NetworkInterface::NetworkInterface( struct ifaddrs* pInterface )
: m_name( pInterface->ifa_name )
, m_socketAddress( *(struct sockaddr_in*) pInterface->ifa_addr )
, m_ipAddressStr{ 0 }
, m_ifFlags( pInterface->ifa_flags ) {
    const void* psin_addr = &m_socketAddress.sin_addr;
    if( psin_addr != nullptr ) {
        inet_ntop( AF_INET, psin_addr, m_ipAddressStr, INET_ADDRSTRLEN );
    }
}

const std::string& NetworkInterface::getName() const {
    return m_name;
}
std::string NetworkInterface::getAddress() const {
    return ( strlen( m_ipAddressStr ) > 0 ? std::string( m_ipAddressStr ) : "" );
}
bool NetworkInterface::hasAddress() const {
    return ( strlen( m_ipAddressStr ) > 0 );
}
bool NetworkInterface::isUp() const {
    return ( m_ifFlags & IFF_UP );
}
bool NetworkInterface::isLoopback() const {
    return ( m_ifFlags & IFF_LOOPBACK );
}
bool NetworkInterface::isWired() const {
    return ( m_name.find( "eth" ) == 0 || m_name.find( "enp" ) == 0 );
}
bool NetworkInterface::isWireless() const {
    return ( m_name.find( "wlan" ) == 0 || m_name.find( "ra" ) == 0 || m_name.find( "apcli" ) == 0 );
}
bool NetworkInterface::doPingTest( const std::string& remoteResource ) {
    const std::string pingCommand = "ping -4 -c 5 -A -w 2 " + remoteResource;
    auto result = system( pingCommand.c_str() );
    return ( result == EXIT_SUCCESS );
}

}  // namespace HardwareTest
}  // namespace HubPlatform
