#include "migration_tool/migration/v1_16/migration1_16.h"

#include "database_client/idbclient.h"
#include "database_client/dbqueryparser.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_16 {

Migration::Migration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool Migration::migrate() {
    auto zigbeeDbName = "db.addons.zigbee.devices";

    auto allDevicesWithoutTime =
        m_dbClient.findAll( zigbeeDbName, JSON( "lastUpdateTime" << JSON( "$exists" << false ) ), nullptr );
    auto currentTime = std::chrono::system_clock::now();
    auto currentTimeValue =
        std::chrono::duration_cast< std::chrono::seconds >( currentTime.time_since_epoch() ).count();

    for( auto& device : allDevicesWithoutTime ) {
        auto id = json_get_int( device, db::ID_FIELD_NAME );

        m_dbClient.update(
            zigbeeDbName,
            JSON( db::ID_FIELD_NAME << id ),
            JSON( "$set" << JSON( "lastUpdateTime" << currentTimeValue << "reachableStateTimeout" << 86400 ) ) );
    }

    return true;
}

}  // namespace v1_16
}  // namespace MigrationTool
}  // namespace HubPlatform
