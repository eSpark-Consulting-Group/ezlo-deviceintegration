#pragma once

#include "uid/cluster_manager/controller_client.h"
#include "service_interfaces/task_loop.h"

#include <string>
#include <vector>
#include <memory>
#include <mutex>

namespace HubPlatform {
namespace Uid {

class Service;
class ClusterManager;

class ControllerConnection final {
public:
    struct Credentials {
        std::string uuid;
        std::vector< std::string > tokens;

        bool valid() const {
            return !uuid.empty() && !tokens.empty();
        }
    };

    struct ConnectionInfo {
        std::vector< std::string > ips;

        bool valid() const {
            return !ips.empty();
        }
    };

public:
    ControllerConnection( ClusterManager& clusterManager,
                          std::string controllerId,
                          Credentials credentials,
                          ConnectionInfo connectionInfo,
                          bool externalControllerConnected );

    ~ControllerConnection();

    const std::string& getControllerId() const;
    Services::TaskLoop& getTaskLoop();
    Uid::Service& getUiService();

    void onCredentialsUpdated( Credentials credentials );
    void onCertificatesUpdated();
    void onConnectionInfoUpdated( ConnectionInfo connectionInfo );

    bool onExternalControllerLoggingIn();
    void onExternalControllerLoggedOut();
    bool isExternalControllerLoggingIn() const;

    bool isConnected() const;
    bool postMessage( const std::string& msg, tiny::ws::client::opcv eOpCode = tiny::ws::client::opcv::text );

public:
    bool enabled() const;
    tiny::ws::security_args getSecurityArgs() const;
    std::unique_lock< std::mutex > lockCertificateFiles();

    std::string getNextAddress();
    std::string getControllerUuid();
    std::string getNextToken();

private:
    void manage();
    void start();
    void stop();

    std::string getCurrentAddress() const;
    std::string getCurrentToken() const;

private:
    ClusterManager& m_clusterManager;
    std::string m_controllerId;
    ControllerClient m_controllerClient;
    Credentials m_credentials;
    ConnectionInfo m_connectionInfo;
    int m_currentAddressIdx;
    int m_currentTokenIdx;

    mutable std::mutex m_mutex;
    bool m_externalControllerConnected;
    bool m_stopping;
};

}  // namespace Uid
}  // namespace HubPlatform
