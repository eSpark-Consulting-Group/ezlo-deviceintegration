#pragma once

#include "system_utils/process/forker.h"

#include <string>
#include <chrono>
#include <stdlib.h>
#include <unistd.h>
#include <memory>
#include <fstream>
#include <future>

namespace HubPlatform {
namespace Ssh {

class TunnelManager {
public:
    /// @struct Data
    /// @brief Data for ssh tunnel creating
    /// @var Data::serverAddress remote address to connect (example: tunnel1-ezlo-cloud.com )
    /// @var Data::serverPort remote port for connection
    /// @var Data::serverUser username for authorization
    /// @var Data::serverPass password for authorization
    /// @var Data::tunnelPort remote port for tunneling
    /// @var Data::ownPort local port for tunneling
    /// @var Data::keepAliveTimeoutSec Check connection with server periodically. Closes connection if connection is
    /// dead. Exit code is either 0 or 5.
    ///                                Closes connection in random time is greater than keepAliveTimeoutSec.
    ///                                If value is 0 then no connection checking. Default value is 0.
    struct Data {
        Data();
        Data( std::string serverAddress,
              int serverPort,
              std::string serverUser,
              std::string serverPass,
              int tunnelPort,
              int ownPort,
              int keepAliveTimeoutSec = 0,
              std::string logFilename = "" );

        std::string serverAddress;
        int serverPort;
        std::string serverUser;
        std::string serverPass;
        int remotePort;
        int ownPort;
        int keepAliveTimeoutSec = 0;
        std::string logFilename;
    };

    struct Result {
        using ErrorMessage = std::string;
        pid_t processId;
        ErrorMessage errorMessage;
    };

    using OnCloseSshTunnel = Process::Forker::OnProcessCompletion;

    TunnelManager();
    TunnelManager( const TunnelManager& ) = delete;
    TunnelManager( TunnelManager&& ) = delete;

    /// @brief create ssh tunnel to remote server. Uses linux system command "ssh"
    /// @param data  Data for ssh tunnel creating
    /// @param onCloseTunnel invokes then ssh tunnel is closed
    pid_t createSshTunnel( const Data& data, OnCloseSshTunnel OnCloseSshTunnel );

    static void killSshTunnel( pid_t processId );

private:
    static std::string prepareCommand( const Data& data, bool isBlocking );
    Process::Forker::Instance m_processForker;
};

}  // namespace Ssh
}  // namespace HubPlatform
