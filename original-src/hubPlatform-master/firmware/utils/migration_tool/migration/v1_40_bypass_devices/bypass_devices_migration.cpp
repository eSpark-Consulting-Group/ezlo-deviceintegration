#include "bypass_devices_migration.h"
#include "database_client/idbclient.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_40_bypass_devices {

namespace DbNames {
const char HaHouseModesDBName[] = "db.firmware.cored.houseModes";
}  // namespace DbNames

namespace Fields {
const char id[] = "_id";
const char bypassDevices[] = "bypassDevices";
};  // namespace Fields

BypassDevicesMigration::BypassDevicesMigration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool BypassDevicesMigration::migrate() {
    LOGGER_DEBUG_RAW( "BypassDevicesMigration::migrate called" );

    auto houseModes = m_dbClient.findAll( DbNames::HaHouseModesDBName, JSON_EMPTY_OBJ, nullptr );
    for( auto& houseMode : houseModes ) {
        houseMode[ Fields::bypassDevices ] = JSON_EMPTY_ARR;

        const std::string id = json_get_str( houseMode, Fields::id );  // what about safety here?
        m_dbClient.update( HaHouseModesDBName, JSON( Fields::id << id ), houseMode );
    }

    return true;
}

}  // namespace v1_40_bypass_devices
}  // namespace MigrationTool
}  // namespace HubPlatform
