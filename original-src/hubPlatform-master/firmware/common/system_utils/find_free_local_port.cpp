#include "system_utils/find_free_local_port.h"
#include "system_utils/process/shared_ram.h"

#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <random>

namespace HubPlatform {
namespace system_utils {

bool isLocalPortFree( std::uint16_t port ) {
    auto isLocalPortFree = [port]( int socketType ) {
        int socketHandle = ::socket( AF_INET, socketType, 0 );
        if( socketHandle == -1 )
            return false;

        struct sockaddr_in sin = {};
        sin.sin_port = htons( port );
        sin.sin_addr.s_addr = 0;
        sin.sin_addr.s_addr = INADDR_ANY;
        sin.sin_family = AF_INET;

        const auto isPortFree = ::bind( socketHandle, (struct sockaddr*) &sin, sizeof( struct sockaddr_in ) ) == 0;
        close( socketHandle );

        return isPortFree;
    };
    return isLocalPortFree( SOCK_STREAM ) && isLocalPortFree( SOCK_DGRAM );  // TODO: SOCK_DGRAM check may not work.
}

/// @brief Get unique free local port.
/// IT IS NOT PROCESS SAFE. USE WITH Process::SharedMutex
/// Example:
/// {
///     Process::SharedMutex sharedMutex( "uniquePortMutex" );
///     std::guard_lock< Process::SharedMutex > lock( sharedMutex );
///
///     auto freePort = getUniqueFreeLocalPort();
///     server.start( freePort );
/// }
///
std::uint16_t getUniqueFreeLocalPort() {
    static const std::uint16_t min = 9000;
    static const std::uint16_t max = 20000;
    Process::SharedRamWrapper< std::uint16_t > sharedRam( "uniquePortRam" );
    std::uint16_t& freePort = sharedRam.get();
    if( freePort < min || freePort > max ) {
        freePort = min;
    }
    freePort += 2;  // spread takes two ports in sequence
    for( ; freePort < max; freePort += 2 ) {
        if( isLocalPortFree( freePort ) ) {
            return freePort;
        }
    }
    throw FreeLocalPortException( std::string( "Cannot find available local port for range: " ) +
                                  std::to_string( min ) + ":" + std::to_string( max ) );
}

}  // namespace system_utils
}  // namespace HubPlatform
