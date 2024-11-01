#include "luad/modules/network/network_utils.h"
#include <netdb.h>
#include <net/if_arp.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sstream>

namespace HubPlatform {
namespace LuaNetwork {

std::string getHostName( const IAddress& address ) {
    char hbuf[ NI_MAXHOST ];
    auto socketAddress = address.getSocketAddress();
    if( getnameinfo( &socketAddress.address, socketAddress.size, hbuf, sizeof( hbuf ), nullptr, 0, NI_NAMEREQD ) ) {
        return std::string();
    }
    else {
        return hbuf;
    }
}

std::string getMacAddress( const IAddress& address, const std::string& networkInterface ) {
    class CloseSocket {
    public:
        CloseSocket( int socket )
        : socket( socket ) {}
        ~CloseSocket() {
            ::close( socket );
        }

    private:
        int socket;
    };
    int socket;
    struct arpreq areq;

    if( ( socket = ::socket( AF_INET, SOCK_DGRAM, 0 ) ) == -1 ) {
        return std::string();
    }
    CloseSocket closeSocket( socket );
    memset( &areq, 0, sizeof( areq ) );
    areq.arp_pa = address.getSocketAddress().address;
    strncpy( areq.arp_dev, networkInterface.c_str(), 15 );
    if( ioctl( socket, SIOCGARP, (caddr_t) &areq ) == -1 ) {
        return std::string();
    }
    char buff[ 25 ];
    unsigned char* ptr = reinterpret_cast< unsigned char* >( &areq.arp_ha.sa_data );
    sprintf( buff,
             "%02X:%02X:%02X:%02X:%02X:%02X",
             ( ptr[ 0 ] & 0xff ),
             ( ptr[ 1 ] & 0xff ),
             ( ptr[ 2 ] & 0xff ),
             ( ptr[ 3 ] & 0xff ),
             ( ptr[ 4 ] & 0xff ),
             ( ptr[ 5 ] & 0xff ) );
    return buff;
}

}  // namespace LuaNetwork
}  // namespace HubPlatform
