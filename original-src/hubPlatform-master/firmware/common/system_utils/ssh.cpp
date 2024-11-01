#include "system_utils/ssh.h"
#include "system_utils/system.h"
#include "log_utils/ezlo_log.h"

#include <future>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <signal.h>

namespace HubPlatform {
namespace Ssh {

std::string escapeSymbolsByFirst( const std::string& source, const char* symbols, size_t escapesCount ) {
    std::string result;
    for( const auto c : source ) {
        if( strchr( symbols, c ) ) {
            result.append( escapesCount, *symbols );
        }
        result += c;
    }
    return result;
}

TunnelManager::TunnelManager()
: m_processForker( Process::Forker::getInstance() ) {}

pid_t TunnelManager::createSshTunnel( const Data& data, OnCloseSshTunnel OnCloseSshTunnel ) {
    return m_processForker->fork( prepareCommand( data, /*isBlocking*/ true ), std::move( OnCloseSshTunnel ) );
}

void TunnelManager::killSshTunnel( pid_t processId ) {
    Process::Forker::kill( processId, SIGTERM );
}

std::string TunnelManager::prepareCommand( const TunnelManager::Data& data, bool isBlocking ) {
    /* Note,
     * 1. Tool sshpass (v1.05 and v1.06 are checked) has issue on arm device (1.05 is unstable and often fails without
     * obvious reason, 1.06 always fails). So we use another way instead. ssh client has following logic: if /dev/tty is
     * open it reads password from tty. Otherwise, it checks env var DISLAY. If DISPLAY is not 0 it reads env var
     * SSH_ASKPASS and reads password from stdout of ./SSH_ASKPASS executable. So create temporary script that gets
     * password on execution. To avoid opened /dev/tty ssh should be run in new session (setsid)
     *
     * 2. Security. Actually password passing is not secure. Safety must be provided on server (minimal user permissions
     * and changing of password right after tunnel establishing).
     *
     * 3. Autoclosing. Ensure that traffic is transmitted at a certain interval in seconds.
     * This is useful for working around firewalls or routers that drop connections after a certain period of
     * inactivity. The trade-off is that a session may be closed if there is a temporary lapse of network connectivity.
     * A setting if 0 disables keepalives.
     */

    /* usage:
     *     set -e
     *     # create temporary script
     *     tempPasswordProvider=`mktemp`
     *     chmod +x $tempPasswordProvider
     *
     *     # write commands to script: output password and remove itself
     *     echo "echo \"pwd\"; rm ${tempPasswordProvider};" > $tempPasswordProvider
     *
     *     # create ssh tunnel in new session with askpass script
     *     SSH_ASKPASS_ALWAYS=1 SSH_ASKPASS=$tempPasswordProvider \
     *         ssh -p 8822 -o ExitOnForwardFailure=yes -o StrictHostKeyChecking=no -y -y -T -R 11190:localhost:22 -K 30
     * user@server.com
     */

    std::string detachProcess;
    if( !isBlocking ) {
        detachProcess = " -f";
    }

    std::string logSwitchParameter = " -y";
    std::string logFileParameter;

    if( !data.logFilename.empty() ) {
        logSwitchParameter = " -v";
        logFileParameter = " exit 2>&1 | while read line; do printf "
                           "'%s %s\\n' \"$(date '+%F %T')\" \"$line\"; done >> " +
                           data.logFilename;
    }

    const auto portForwardingOptions =
        " -o ExitOnForwardFailure=yes -o StrictHostKeyChecking=no " + detachProcess + logSwitchParameter + " -T -N -R ";
    const auto keepAliveOptions =
        " -o ServerAliveCountMax=1 -o ServerAliveInterval=" + std::to_string( data.keepAliveTimeoutSec );

    const auto sshCommand = " ssh -p " + std::to_string( data.serverPort ) + portForwardingOptions +
                            std::to_string( data.remotePort ) + ":localhost:" + std::to_string( data.ownPort ) +
                            keepAliveOptions + " " + data.serverUser + "@" + data.serverAddress + logFileParameter;

    std::string passwordCommand;
#if defined( __HUB_TYPE_LOCAL__ ) || defined( __HUB_TYPE_VHUB__ )
    if( !data.serverPass.empty() ) {
        passwordCommand = " exec sshpass -p '" + escapeSymbolsByFirst( data.serverPass, R"(\"`$)", 2u ) + "'";
    }
#else
    if( !data.serverPass.empty() ) {
        passwordCommand = "tempPasswordProvider=`mktemp`;"
                          "chmod +x $tempPasswordProvider;"
                          "echo \"#!/bin/sh\necho \\\"" +
                          escapeSymbolsByFirst( data.serverPass, R"(\"`$)", 2u ) +
                          "\\\"\nrm ${tempPasswordProvider}\" > $tempPasswordProvider;"
                          "SSH_ASKPASS_ALWAYS=1 SSH_ASKPASS=$tempPasswordProvider DISPLAY= ; exec";
    }
    else {
        passwordCommand = " exec";
    }
#endif
    const auto command = "set -e; " + passwordCommand + sshCommand;
    return command;
}

TunnelManager::Data::Data()
: serverPort( 0 )
, remotePort( 0 )
, ownPort( 0 )
, keepAliveTimeoutSec( 0 ) {}

TunnelManager::Data::Data( std::string serverAddress,
                           int serverPort,
                           std::string serverUser,
                           std::string serverPass,
                           int tunnelPort,
                           int ownPort,
                           int keepAliveTimeoutSec,
                           std::string logFilename )
: serverAddress( serverAddress )
, serverPort( serverPort )
, serverUser( serverUser )
, serverPass( serverPass )
, remotePort( tunnelPort )
, ownPort( ownPort )
, keepAliveTimeoutSec( keepAliveTimeoutSec )
, logFilename( logFilename ) {}

}  // namespace Ssh
}  // namespace HubPlatform
