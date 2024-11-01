#pragma once

#include "json_utils/json_fwd.h"

namespace HubPlatform {
namespace CoreD {

class PolicyUpdater {
public:
    static JsonObject getPolicyUpdateBroadcastData( const JsonObject& updatedData, const JsonObject& savedData );
    static JsonObject getPolicyUpdateData( const JsonObject& dbItem, const Json& params );
    static void ensurePredefinedPoliciesInDb( IDbClient& dbClient, const ConfigReader& configReader );
};

}  // namespace CoreD
}  // namespace HubPlatform
