#include "migration_tool/migration/v1_6/migration1_6.h"

#include "database_client/idbclient.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_6 {

Migration::Migration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool Migration::migrate() {
    auto GatewaysDBName = "db.firmware.cored.gateways";
    const std::string status = "status";
    m_dbClient.update( GatewaysDBName,
                       JSON( status << JSON( "$exists" << false ) ),
                       JSON( "$set" << JSON( status << "not_started" ) ) );
    return true;
}

}  // namespace v1_6
}  // namespace MigrationTool
}  // namespace HubPlatform
