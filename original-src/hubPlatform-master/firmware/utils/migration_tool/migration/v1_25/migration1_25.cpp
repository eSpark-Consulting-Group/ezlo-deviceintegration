#include "migration_tool/migration/v1_25/migration1_25.h"

#include "system_utils/environment.h"
#include "database_client/idbclient.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_25 {

Migration::Migration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool Migration::migrate() {
    m_dbClient.drop( "db.firmware.networkd.cloud.ssh.tunnels" );
    return true;
}

}  // namespace v1_25
}  // namespace MigrationTool
}  // namespace HubPlatform