#pragma once

#include "database_client/idbclient.h"
#include "json_utils/json_fwd.h"
#include "jsonrpc/error_codes.h"

#include <string>
#include <set>

namespace HubPlatform {
namespace MQTTGateway {
namespace Utils {

Json buildErrorResponseBody( Errors::Error error, const std::string& reason = "" );

Json buildMQTTResponse( const std::string& group, const std::string& messageUUID, Json responseBody );
Json buildMQTTErrorResponse( const std::string& group,
                             const std::string& messageUUID,
                             Errors::Error error,
                             const std::string& reason = "" );

std::set< std::string > readClientsFromDB( IDbClient& dbClient );
void addClientToDB( IDbClient& dbClient, const std::string& clientId );
void removeClientFromDB( IDbClient& dbClient, const std::string& clientId );

}  // namespace Utils
}  // namespace MQTTGateway
}  // namespace HubPlatform
