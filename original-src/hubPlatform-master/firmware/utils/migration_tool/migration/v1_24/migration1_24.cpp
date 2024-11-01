#include "migration_tool/migration/v1_24/migration1_24.h"

#include "system_utils/environment.h"
#include "database_client/idbclient.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_24 {

Migration::Migration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool Migration::migrate() {
    auto ProvisioningListDBName = "db.addons.zwave.provisioning";
    const std::string setting = "inclusionSetting";
    m_dbClient.update( ProvisioningListDBName,
                       JSON( setting << JSON( "$exists" << false ) ),
                       JSON( "$set" << JSON( setting << "pending" ) ) );
    return true;
}

}  // namespace v1_24
}  // namespace MigrationTool
}  // namespace HubPlatform
