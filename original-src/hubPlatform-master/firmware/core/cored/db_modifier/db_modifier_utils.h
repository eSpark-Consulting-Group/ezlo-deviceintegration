#pragma once
#include "scenes_manager/scenes_engine/helpers/json_helper.h"
#include "json_utils/json_path.h"
#include "config_reader/config_reader.h"
#include "database_client/idbclient.h"

namespace HubPlatform {
namespace CoreD {
namespace DBModifierUtils {
Json addProtectionConfig( const Json& data, const Json& deviceInfo );
std::string getProtectConfig( IDbClient& dbClient, const std::string& itemId );

}  // namespace DBModifierUtils
}  // namespace CoreD
}  // namespace HubPlatform
