#pragma once

#include "json_utils/json_fwd.h"
#include "jsonrpc/message.h"
#include "database_client/idbclient.h"

#include <mutex>
#include <condition_variable>
#include <deque>
#include <set>
#include <memory>

namespace HubPlatform {
namespace Tests {

class GatewaysChecker {
public:
    GatewaysChecker( IDbClient& dbClient );
    void checkGatewayReady( const std::set< std::string >& pluginIds );
    void handleMessage( rpc::Message& message );

private:
    IDbClient& m_dbClient;
    std::mutex m_broadcastsMutex;
    std::condition_variable m_broadcastsCondVar;
    std::deque< JsonObject > m_broadcastMessages;
};

}  // namespace Tests
}  // namespace HubPlatform
