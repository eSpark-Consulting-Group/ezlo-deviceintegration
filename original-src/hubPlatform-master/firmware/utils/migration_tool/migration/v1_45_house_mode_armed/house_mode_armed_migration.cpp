#include "house_mode_armed_migration.h"
#include "database_client/idbclient.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_45_house_mode_armed {

namespace DbNames {
const char HaHouseModesDBName[] = "db.firmware.cored.houseModes";
}  // namespace DbNames

namespace Fields {
const char id[] = "_id";
const char armed[] = "armed";
};  // namespace Fields

HouseModeArmedMigration::HouseModeArmedMigration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool HouseModeArmedMigration::migrate() {
    LOGGER_DEBUG_RAW( "HouseModeArmedMigration::migrate called" );

    auto houseModes = m_dbClient.findAll( DbNames::HaHouseModesDBName, JSON_EMPTY_OBJ, nullptr );
    for( auto& houseMode : houseModes ) {
        const std::string id = json_get_str( houseMode, Fields::id );
        const bool isArmed = id != "1";
        houseMode[ Fields::armed ] = isArmed;

        m_dbClient.update( DbNames::HaHouseModesDBName, JSON( Fields::id << id ), houseMode );
    }

    return true;
}

}  // namespace v1_45_house_mode_armed
}  // namespace MigrationTool
}  // namespace HubPlatform
