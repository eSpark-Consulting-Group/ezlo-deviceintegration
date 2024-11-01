#include "migration_tool/migration/v1_38_network_statistic/network_statistic_migration.h"

#include "database_client/idbclient.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_38_network_statistic {

Migration::Migration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool Migration::migrate() {
    m_dbClient.insert( "db.firmware.networkd.statistic.settings",
                       JSON( "_id"
                             << "settings"
                             << "storageSize" << 30 ) );
    return true;
}

}  // namespace v1_38_network_statistic
}  // namespace MigrationTool
}  // namespace HubPlatform
