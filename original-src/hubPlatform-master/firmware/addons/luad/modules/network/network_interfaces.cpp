#include "luad/modules/network/network_interfaces.h"
#include <sys/socket.h>
#include <netdb.h>

namespace HubPlatform {
namespace LuaNetwork {

NetworkInterfaces::NetworkInterfaces()
: m_ifaddrs( nullptr ) {}

NetworkInterfaces::~NetworkInterfaces() {
    freeifaddrs( m_ifaddrs );
}

NetworkInterfaces::InterfaceNames NetworkInterfaces::getInterfaceNames() {
    getIfaddrs();
    InterfaceNames names;
    for( ifaddrs* ifa = m_ifaddrs; ifa != nullptr; ifa = ifa->ifa_next ) {
        if( ifa->ifa_addr == nullptr ) {
            continue;
        }
        names.emplace( ifa->ifa_name );
    }
    return names;
}

void NetworkInterfaces::getIfaddrs() {
    if( !m_ifaddrs && getifaddrs( &m_ifaddrs ) == -1 ) {
        throw NetworkInterfacesError( std::string( "Can't get interfaces: " ) + strerror( errno ) );
    }
}

}  // namespace LuaNetwork
}  // namespace HubPlatform
