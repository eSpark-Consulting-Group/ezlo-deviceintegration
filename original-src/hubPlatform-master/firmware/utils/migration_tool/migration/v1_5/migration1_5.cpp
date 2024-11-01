#include "migration_tool/migration/v1_5/migration1_5.h"

#include "database_client/idbclient.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_5 {

Migration::Migration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool Migration::migrate() {
    auto DevicesDBName = "db.firmware.cored.devices";
    const std::string serviceNotification = "serviceNotification";
    m_dbClient.update( DevicesDBName,
                       JSON( serviceNotification << JSON( "$exists" << false ) ),
                       JSON( "$set" << JSON( serviceNotification << false ) ) );
    return true;
}

}  // namespace v1_5
}  // namespace MigrationTool
}  // namespace HubPlatform
