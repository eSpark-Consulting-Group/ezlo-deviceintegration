#include "migration_tool/migration/v1_55_protect_config/protect_config.h"

#include "system_utils/environment.h"
#include "database_client/idbclient.h"
#include "jsonrpc/jsonrpc.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_55_protect_config {

Migration::Migration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool Migration::migrate() {
    auto DevicesDBName = "db.firmware.cored.devices";
    const std::string ProtectConfig = "protectConfig";
    const std::string ProtectConfigDefault = "default";
    auto fields = JSON( "_id" << 1 );

    m_dbClient.update( DevicesDBName,
                       JSON( ProtectConfig << JSON( "$exists" << false ) ),
                       JSON( "$set" << JSON( ProtectConfig << ProtectConfigDefault ) ) );

    return true;
}

}  // namespace v1_55_protect_config
}  // namespace MigrationTool
}  // namespace HubPlatform