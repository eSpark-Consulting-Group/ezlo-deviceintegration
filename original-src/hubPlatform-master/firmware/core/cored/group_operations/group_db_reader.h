#pragma once

#include "json_utils/json_wrapper.h"

#include <string>
#include <map>
#include <set>
#include <vector>
#include <functional>

class IDbClient;

namespace HubPlatform {
namespace CoreD {
namespace GroupOperations {

// DONE: EFWR-4000: R.1 Extract reader's functions to a separate class
class DbReader {
public:
    static JsonArray readDeviceGroupsFromDatabase( IDbClient& dbClient );
    static JsonObject fetchSingleDeviceGroup( IDbClient& dbClient, const JsonObject& condition );
    static JsonArray readItemGroupsFromDatabase( IDbClient& dbClient );
    static JsonObject fetchSingleItemGroup( IDbClient& dbClient, const JsonObject& condition );
};

}  // namespace GroupOperations
}  // namespace CoreD
}  // namespace HubPlatform