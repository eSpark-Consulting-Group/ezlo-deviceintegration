#include "migration_tool/migration/v1_8/migration1_8.h"

#include "system_utils/environment.h"
#include "system_utils/filesystem.h"
#include "database_client/idbclient.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_8 {

Migration::Migration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool Migration::migrate() {
    const std::set< std::string > collectionsToRemove = { "db.firmware.uid.user" };

    for( auto& collectionName : collectionsToRemove ) {
        if( m_dbClient.exist( collectionName ) ) {
            m_dbClient.drop( collectionName );
        }
    }

    filesystem::deleteFolder( Environment::getFirmwareRootFolder() + "/certs" );
    return true;
}

}  // namespace v1_8
}  // namespace MigrationTool
}  // namespace HubPlatform
