#include "migration_tool/migration/v1_12/migration1_12.h"

#include "system_utils/environment.h"
#include "database_client/idbclient.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_12 {

Migration::Migration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool Migration::migrate() {
    auto DevicesDBName = "db.firmware.cored.devices";
    const std::string status = "status";
    m_dbClient.update(
        DevicesDBName, JSON( status << JSON( "$exists" << false ) ), JSON( "$set" << JSON( status << "idle" ) ) );
    return true;
}

}  // namespace v1_12
}  // namespace MigrationTool
}  // namespace HubPlatform