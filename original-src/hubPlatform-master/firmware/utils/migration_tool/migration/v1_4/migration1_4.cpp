#include "migration_tool/migration/v1_4/migration1_4.h"

#include "database_client/idbclient.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_4 {

Migration::Migration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool Migration::migrate() {
    auto DevicesDBName = "db.firmware.cored.devices";
    const std::string ready = "ready";
    m_dbClient.update(
        DevicesDBName, JSON( ready << JSON( "$exists" << false ) ), JSON( "$set" << JSON( ready << true ) ) );
    return true;
}

}  // namespace v1_4
}  // namespace MigrationTool
}  // namespace HubPlatform
