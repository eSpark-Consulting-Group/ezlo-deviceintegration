#include "migration_tool/migration/v1_11/migration1_11.h"

#include "system_utils/environment.h"
#include "system_utils/filesystem.h"
#include "database_client/idbclient.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_11 {

Migration::Migration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool Migration::migrate() {
    const std::set< std::string > collectionsToRemove = { "temp.db.firmware.scenes_manager.items" };
    for( auto& collectionName : collectionsToRemove ) {
        if( m_dbClient.exist( collectionName ) ) {
            m_dbClient.drop( collectionName );
        }
    }
    return true;
}

}  // namespace v1_11
}  // namespace MigrationTool
}  // namespace HubPlatform
