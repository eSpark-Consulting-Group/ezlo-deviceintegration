#include "migration_tool/migration/v1_19/migration1_19.h"

#include "database_client/idbclient.h"
#include "database_client/dbqueryparser.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_19 {

Migration::Migration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool Migration::migrate() {
    auto GatewaysDBName = "db.firmware.cored.gateways";

    auto manualDeviceAddingField = "manualDeviceAdding";
    auto defaultValue = "wizard";

    m_dbClient.update( GatewaysDBName,
                       JSON( manualDeviceAddingField << JSON( "$exists" << false ) ),
                       JSON( "$set" << JSON( manualDeviceAddingField << defaultValue ) ) );

    return true;
}

}  // namespace v1_19
}  // namespace MigrationTool
}  // namespace HubPlatform
