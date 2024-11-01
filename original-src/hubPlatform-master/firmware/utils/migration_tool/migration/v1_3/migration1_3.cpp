#include "migration_tool/migration/v1_3/migration1_3.h"

#include "database_client/idbclient.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_3 {

Migration::Migration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool Migration::migrate() {
    auto DevicesDBName = "db.firmware.cored.devices";
    const std::string security = "security";
    m_dbClient.update(
        DevicesDBName, JSON( security << JSON( "$exists" << false ) ), JSON( "$set" << JSON( security << "no" ) ) );
    return true;
}

}  // namespace v1_3
}  // namespace MigrationTool
}  // namespace HubPlatform
