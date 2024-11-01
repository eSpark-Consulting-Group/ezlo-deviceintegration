#include "migration_tool/migration/v1_9/migration1_9.h"

#include "system_utils/environment.h"
#include "system_utils/filesystem.h"
#include "database_client/idbclient.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_9 {

Migration::Migration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool Migration::migrate() {
    const std::string collectionToRemove = { "temp.db.firmware.scenes_manager.items" };

    if( m_dbClient.exist( collectionToRemove ) ) {
        m_dbClient.drop( collectionToRemove );
    }
    return true;
}

}  // namespace v1_9
}  // namespace MigrationTool
}  // namespace HubPlatform
