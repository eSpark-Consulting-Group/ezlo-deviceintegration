#pragma once

#include "service_interfaces/service.h"
#include "jsonrpc/message.h"

#include <string>

namespace HubPlatform {
namespace Uid {

class Service;

class HubOfflineCommandsHandler final {
public:
    explicit HubOfflineCommandsHandler( Uid::Service& hostService );

    void handleCredentialsUpdatedRequest( rpc::Message& rpcMessage );
    void handleCloudAccessKeysControllerSyncRequest( rpc::Message& rpcMessage );
    void handleOfflineAnonymousAccessEnabledSetRequest( rpc::Message& rpcMessage );
    void handleOfflineInsecureAccessEnabledSetRequest( rpc::Message& rpcMessage );

    void requestCredentials();
    static bool anonymousAccessEnabled();
    static bool authenticateUser( const std::string& uuid, const std::string& key );
    static bool authenticateController( const std::string& uuid, const std::string& key );

public:
    static int getCurrentCredentialsVersion( IDbClient& dbClient );
    static JsonArray getCurrentCredentials( IDbClient& dbClient );

private:
    Uid::Service& m_hostService;
};

}  // namespace Uid
}  // namespace HubPlatform