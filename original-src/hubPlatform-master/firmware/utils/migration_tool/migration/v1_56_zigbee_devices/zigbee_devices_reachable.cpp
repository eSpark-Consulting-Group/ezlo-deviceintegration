#include "migration_tool/migration/v1_56_zigbee_devices/zigbee_devices_reachable.h"

#include "database_client/idbclient.h"
#include "database_client/dbqueryparser.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_56_zigbee_devices {

Migration::Migration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool Migration::migrate() {
    auto zigbeeDbName = "db.addons.zigbee.devices";

    const auto fieldsToReturn = JSON( "_id"
                                      << "_id"
                                      << "is_battery_powered"
                                      << "is_battery_powered"
                                      << "manufacturer_id"
                                      << "manufacturer_id"
                                      << "reachable_state_timeout"
                                      << "reachable_state_timeout" );
    auto allDevices = m_dbClient.findAll( zigbeeDbName, JSON_EMPTY_OBJ, &fieldsToReturn );

    for( auto& device : allDevices ) {
        auto id = json_get_int( device, db::ID_FIELD_NAME );

        auto reachableStateTimeout = json_get_uint( device, "reachable_state_timeout" );
        device[ "reachable_state_timeout" ] = reachableStateTimeout * 2;

        auto manufacturerId = json_get_str( device, "manufacturer_id" );
        std::transform( manufacturerId.begin(), manufacturerId.end(), manufacturerId.begin(), ::tolower );

        if( manufacturerId == "centralite" && json_get_bool( device, "is_battery_powered" ) == true ) {
            device[ "is_sleepy" ] = true;
        }

        m_dbClient.update( zigbeeDbName, JSON( db::ID_FIELD_NAME << id ), JSON( "$set" << device ) );
    }

    return true;
}

}  // namespace v1_56_zigbee_devices
}  // namespace MigrationTool
}  // namespace HubPlatform
