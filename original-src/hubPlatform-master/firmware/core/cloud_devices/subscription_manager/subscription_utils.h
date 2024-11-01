#pragma once

#include "cored/remote_interface/cored_interface.h"
#include "database_client/idbclient.h"
#include "json_utils/json_utils.h"

#include <map>
#include <set>

namespace HubPlatform {
namespace CloudDevices {
namespace SubscriptionUtils {

using ItemValueMap = std::map< std::string, Json >;

Json readSubscriptionFromDb( IDbClient& dbClient, const std::string& abstractUUID );
Json readAllSubscriptionsFromDb( IDbClient& dbClient );
void removeSubscriptionFromDb( IDbClient& dbClient, const std::string& abstractUUID );
void storeSubscriptionToDb( IDbClient& dbClient, const Json& subscription );
void removeSubscriptionItemFromDb( IDbClient& dbClient, const std::string& abstractUUID, const std::string& id );

Json createSubscriptionMessage( const JsonObject& subscriptionsTable, const ItemValueMap& itemValueMap );

class SubscriptionBuilder {
public:
    using ResponseTypes = std::set< std::string >;

public:
    static Json formatControllerSubscribeAbstractsCommand( Json params );

    static JsonObject formatMapping( const Json& dbMapping,
                                     const ItemValueMap& itemValueMap,
                                     ResponseTypes& responseTypes );
    static JsonObject formatAbstract( const JsonObject& data,
                                      const ItemValueMap& itemValueMap,
                                      ResponseTypes& responseTypes );

    static Json formatResponseType( const ResponseTypes& responseTypes );
};

}  // namespace SubscriptionUtils
}  // namespace CloudDevices
}  // namespace HubPlatform
