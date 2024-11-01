#include "networkd/ssh_tunnel/ssh_defs.h"
#include "networkd/ssh_tunnel/ssh_defs_resources.h"

#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"

#include <map>

namespace HubPlatform {
namespace Network {
namespace SshTunnel {

std::string toString( Status status ) {
    switch( status ) {
    case Status::Connected:
        return StatusConnected;
    case Status::Connecting:
        return StatusConnecting;
    case Status::Failed:
        return StatusFailed;
    case Status::Disconnected:
        return StatusDisconnected;
    case Status::Rejected:
        return StatusRejected;
    }

    LOGGER_ERROR( cc::error( "Unhandled enum value from 'SshTunnel::Status': " ) +
                  cc::note( std::to_string( static_cast< int >( status ) ) ) );
    return std::string();
}

}  // namespace SshTunnel
}  // namespace Network
}  // namespace HubPlatform
