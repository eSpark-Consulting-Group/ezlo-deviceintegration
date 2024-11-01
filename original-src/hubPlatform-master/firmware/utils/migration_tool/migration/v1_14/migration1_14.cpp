#include "migration_tool/migration/v1_14/migration1_14.h"

#include "database_client/idbclient.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_14 {

Migration::Migration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool Migration::migrate() {
    auto GatewaysDBName = "db.firmware.cored.gateways";
    m_dbClient.update( GatewaysDBName, JSON( "api" << JSON( "$exists" << true ) ), []( Json& gateway ) {
        json_remove( gateway, "api" );
    } );
    return true;
}

}  // namespace v1_14
}  // namespace MigrationTool
}  // namespace HubPlatform
