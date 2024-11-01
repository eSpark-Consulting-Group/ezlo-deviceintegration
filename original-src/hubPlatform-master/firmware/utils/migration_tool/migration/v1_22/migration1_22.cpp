#include "migration_tool/migration/v1_22/migration1_22.h"

#include "database_client/idbclient.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_22 {

Migration::Migration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool Migration::migrate() {
    m_dbClient.fixBrokenPrimaryKeys( "db.firmware.cored.gateways" );
    m_dbClient.fixBrokenPrimaryKeys( "db.firmware.uid.items" );
    m_dbClient.fixBrokenPrimaryKeys( "db.firmware.cored.devices" );

    return true;
}

}  // namespace v1_22
}  // namespace MigrationTool
}  // namespace HubPlatform
