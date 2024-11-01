//
// Created by nffs on 02.09.15.
//

#include "database_client/dbid.h"

#include <sys/ioctl.h>
#include <stdlib.h>
#include <iomanip>
#include <algorithm>

#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sstream>

#if( !defined SIOCGIFHWADDR )
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <net/if_dl.h>
#endif
#include <unistd.h>
#include <atomic>

namespace db {

const char* getInterfaceName() {
#ifdef __HUB_TYPE_G150__
    return "apcli0";
#else
    return "wlan0";
#endif
}

std::vector< unsigned char > DbId::genOIDData() {
    std::vector< unsigned char > oid;
    const std::vector< unsigned char > mac = getMacAddress( getInterfaceName() );
    static std::atomic< int > nextCtr( getRandomData() );
    ++nextCtr;
    std::vector< unsigned char > counter = { uint8_t( nextCtr >> 16 ), uint8_t( nextCtr >> 8 ), uint8_t( nextCtr ) };
    addIntToCharVector( (int) std::time( nullptr ), oid );
    oid.insert( oid.end(), mac.begin(), mac.end() );
    addIntToCharVector( getpid(), oid, 2 );
    oid.insert( oid.end(), counter.begin(), counter.end() );
    return oid;
}

std::string DbId::genOID() {
    return getHexString( genOIDData() );
}

std::vector< unsigned char > DbId::getMacAddress( const std::string& interface ) {
    std::vector< unsigned char > returnedValue;
#if( defined SIOCGIFHWADDR )
    int fd;
    struct ifreq ifr;
    memset( &ifr, 0, sizeof( ifreq ) );
    fd = socket( AF_INET, SOCK_DGRAM, 0 );
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy( ifr.ifr_name, interface.c_str(), IFNAMSIZ - 1 );
    ioctl( fd, SIOCGIFHWADDR, &ifr );
    close( fd );
    char* mac = ifr.ifr_hwaddr.sa_data;
    returnedValue.insert( returnedValue.end(), mac + 3, mac + 6 );
#else
    ifaddrs* iflist;
    // bool found = false;
    if( getifaddrs( &iflist ) == 0 ) {
        for( ifaddrs* cur = iflist; cur; cur = cur->ifa_next ) {
            if( ( cur->ifa_addr->sa_family == AF_LINK ) && ( cur->ifa_name == interface ) && cur->ifa_addr ) {
                sockaddr_dl* sdl = (sockaddr_dl*) cur->ifa_addr;
                returnedValue.insert( returnedValue.end(), LLADDR( sdl ), ( LLADDR( sdl ) ) + sdl->sdl_alen );
                // found = true;
                break;
            }
        }

        freeifaddrs( iflist );
    }
#endif
    return returnedValue;
}

int DbId::getRandomData() {
    int size = 4;
    unsigned char* data[ 4 ];
    FILE* fp;
    fp = fopen( "/dev/urandom", "r" );
    fread( data, 1, size, fp );
    fclose( fp );
    return *reinterpret_cast< int* >( data );
}

void DbId::addIntToCharVector( int value, std::vector< unsigned char >& vector, int size ) {
    std::vector< unsigned char > vec;
    for( int i = 0; i < size; i++ ) {
        vec.push_back( value );
        value = value >> 8;
    }
    std::reverse( vec.begin(), vec.end() );
    vector.insert( vector.end(), vec.begin(), vec.end() );
}

std::string DbId::getHexString( const std::vector< unsigned char >& data, bool upperCase ) {
    std::ostringstream result;
    result << std::hex << std::setfill( '0' ) << ( upperCase ? std::uppercase : std::nouppercase );
    for( const auto& dataByte : data )
        result << std::setw( 2 ) << (int) dataByte;
    return result.str();
}

}  // namespace db
