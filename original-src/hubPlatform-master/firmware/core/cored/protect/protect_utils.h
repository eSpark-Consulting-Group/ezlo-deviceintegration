#pragma once
#include "scenes_manager/scenes_engine/helpers/json_helper.h"
#include "json_utils/json_path.h"
#include "config_reader/config_reader.h"

class IDbClient;
namespace HubPlatform {
namespace CoreD {
namespace Protect {
namespace ProtectUtils {

void ensurePredefinedContactIdsInDb( IDbClient& dbClient, const ConfigReader& configReader );
void ensurePredefinedSecurityConfigsInDb( IDbClient& dbClient, const ConfigReader& configReader );
JsonObject getSecurityConfigObject( IDbClient& dbClient, const std::string& id );
JsonObject getContactIdObject( IDbClient& dbClient, const std::string& id );

}  // namespace ProtectUtils
}  // namespace Protect
}  // namespace CoreD
}  // namespace HubPlatform