#include "networkd/ssh_tunnel/ssh_state_wait_timeout.h"
#include "networkd/ssh_tunnel/ssh_state_request_token.h"
#include "common_utils/cpp14.h"
#include "log_utils/ezlo_log.h"
#include <tiny/tiny_consolecolors.h>

namespace HubPlatform {
namespace Network {
namespace SshTunnel {

WaitTimeoutState::WaitTimeoutState( SshTunnel& sshTunnel )
: QueueState( sshTunnel ) {}

void WaitTimeoutState::init() {
    auto timeout = m_sshTunnel.getTimeout();
    Log::info( cc::info( "Wait timeout " + cc::note( std::to_string( timeout.count() ) ) + " seconds" ) );
    auto timer = m_sshTunnel.createTimer( [this]( Services::PostponedTask& ) {
        m_sshTunnel.setState( std::make_unique< RequestTokenState >( m_sshTunnel ) );
    } );
    timer->start( timeout, false );
}

}  // namespace SshTunnel
}  // namespace Network
}  // namespace HubPlatform
