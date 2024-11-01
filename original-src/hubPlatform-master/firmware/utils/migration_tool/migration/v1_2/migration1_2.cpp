#include "migration_tool/migration/v1_2/migration1_2.h"

#include "database_client/idbclient.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_2 {

Migration::Migration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool Migration::migrate() {
    auto HouseModesDBName = "db.firmware.cored.houseModes";
    m_dbClient.update(
        HouseModesDBName, JSON( "disarmedDefault" << true ), JSON( "$set" << JSON( "notifications" << nullptr ) ) );
    m_dbClient.update( HouseModesDBName, JSON_EMPTY_OBJ, JSON( "$set" << JSON( "notificationsDefault" << nullptr ) ) );
    return true;
}

}  // namespace v1_2
}  // namespace MigrationTool
}  // namespace HubPlatform
