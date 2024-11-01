#include "migration_tool/migration/v1_18/migration1_18.h"

#include "database_client/idbclient.h"
#include "database_client/dbqueryparser.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_18 {

Migration::Migration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool Migration::migrate() {
    migrateField( "manufacturer_id" );
    migrateField( "model_id" );

    return true;
}

bool Migration::migrateField( const std::string& fieldName ) {
    auto zigbeeDbName = "db.addons.zigbee.devices";

    auto allDevices = m_dbClient.findAll( zigbeeDbName, JSON( fieldName << "" ), nullptr );

    for( auto& device : allDevices ) {
        auto id = json_get_int( device, db::ID_FIELD_NAME );

        m_dbClient.update(
            zigbeeDbName, JSON( db::ID_FIELD_NAME << id ), JSON( "$set" << JSON( fieldName << "Unknown" ) ) );
    }

    return true;
}

}  // namespace v1_18
}  // namespace MigrationTool
}  // namespace HubPlatform
