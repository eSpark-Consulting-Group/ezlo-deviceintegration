#pragma once

#include "cored/policies/policies.h"
#include "json_utils/json_utils.h"
#include "cored/remote_interface/cored_interface.h"
#include <string>

class IDbClient;

namespace HubPlatform {
namespace CoreD {

class PolicyOperationsHandler {
public:
    using BroadcastCallback = std::function< void( MessageSubClasses messageSubClass, const JsonObject message ) >;

    explicit PolicyOperationsHandler( IDbClient& dbClient,
                                      BroadcastCallback& sendBroadcast,
                                      CoreDRemoteInterface::CoreDInterface& core );
    JsonObject validatePolicyRequest( const JsonObject& message, bool checkRequiredParams );
    JsonObject processPolicyCreateRequest( const JsonObject& params );
    JsonObject processPolicyDeleteRequest( const JsonObject& params );
    JsonObject processPolicyUpdateRequest( const JsonObject& params );
    JsonObject processPolicyGetRequest( const JsonObject& params );
    JsonObject processPolicyListRequest( const JsonObject& params );
    JsonObject processPolicyUpdateBroadcastData( const JsonObject& updatedData, const JsonObject& savedData );

private:
    IDbClient& getDBClient();

private:
    IDbClient& m_dbClient;
    BroadcastCallback m_sendBroadcast;
    CoreDRemoteInterface::CoreDInterface& m_core;
};

}  // namespace CoreD
}  // namespace HubPlatform
