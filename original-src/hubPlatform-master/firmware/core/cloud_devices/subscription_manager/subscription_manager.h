#pragma once

#include "cloud_devices/subscription_manager/subscription_helper_interface.h"
#include "cored/remote_interface/cored_interface.h"

#include "database_client/idbclient.h"
#include "json_utils/json_utils.h"

#include <memory>
#include <list>

namespace HubPlatform {
namespace CloudDevices {

class SubscriptionManager {
public:
    SubscriptionManager( ISubscriptionHelper& helper, IDbClient& dbClient );

    JsonObject addSubscription( const Json& params );
    JsonObject updateSubscription( const Json& params );
    JsonObject deleteSubscription( const Json& params );
    void sendSubscriptionList();
    JsonObject dispatchSubscription( const Json& params );

    void storeSubscriptionRecord( const Json& params );
    void removeSubscriptionRecord( const Json& params );

private:
    ISubscriptionHelper& m_helper;
    IDbClient& m_dbClient;
};

}  // namespace CloudDevices
}  // namespace HubPlatform
