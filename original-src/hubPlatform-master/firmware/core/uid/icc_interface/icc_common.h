#pragma once

#include "database_client/idbclient.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace InterControllerCommunication {
namespace Common {

inline std::string getControllerIdByUUID( const IDbClient& dbClient, const std::string& controllerUuid ) {
    auto fieldsToReturn = JSON( "serial" << 1 );
    auto credentials = dbClient.findOne( CredentialsDBName, JSON( "uuid" << controllerUuid ), &fieldsToReturn );
    if( credentials.empty() ) {
        return {};
    }

    return json_get_str( credentials, "serial" );
}

inline std::string getControllerUUIDById( const IDbClient& dbClient, const std::string& controllerId ) {
    auto fieldsToReturn = JSON( "uuid" << 1 );
    auto credentials = dbClient.findOne( CredentialsDBName, JSON( "serial" << controllerId ), &fieldsToReturn );
    if( credentials.empty() ) {
        return {};
    }

    return json_get_str( credentials, "uuid" );
}

}  // namespace Common
}  // namespace InterControllerCommunication
}  // namespace HubPlatform
