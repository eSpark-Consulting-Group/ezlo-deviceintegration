#include "migration_tool/migration/v1_31/gateway_settings_implementation_migration.h"

#include "database_client/idbclient.h"
#include "database_client/dbqueryparser.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_31 {

Migration::Migration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool Migration::migrate() {
    auto gates = m_dbClient.findAll( gatewaysTableName, JSON_EMPTY_OBJ, nullptr );

    for( auto& gate : gates ) {
        gate[ settingsFieldName ] = 0;
        m_dbClient.insert( gatewaysTableName, gate );
    }

    return true;
}

}  // namespace v1_31
}  // namespace MigrationTool
}  // namespace HubPlatform
