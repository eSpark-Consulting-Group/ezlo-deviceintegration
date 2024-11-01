#include "migration_tool/migration/v1_20/migration1_20.h"

#include "database_client/idbclient.h"
#include "database_client/dbqueryparser.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_20 {

Migration::Migration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool Migration::migrate() {
    m_dbClient.drop( m_dataTableName );

    return true;
}

}  // namespace v1_20
}  // namespace MigrationTool
}  // namespace HubPlatform
