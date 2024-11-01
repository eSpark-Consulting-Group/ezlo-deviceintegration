#include "house_mode_glass_breakage.h"
#include "database_client/idbclient.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_54_house_mode_glass_breakage_sensor {

Migration::Migration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool Migration::migrate() {
    LOGGER_DEBUG_RAW( "HouseModeGlassBreakageSensorMigration::migrate called" );

    auto houseModeSetting =
        m_dbClient.findOne( DbNames::HUBSettings, JSON( Fields::Id << Fields::houseModeEntry ), nullptr );
    auto currentHouseMode = json_get_str( houseModeSetting, Fields::houseModeEntry );
    auto houseMode = m_dbClient.findOne( DbNames::HaHouseModesDBName, JSON( Fields::Id << currentHouseMode ), nullptr );
    auto houseModeArmed = json_get_bool( houseMode, Fields::Armed );
    auto glassSensorDevices =
        m_dbClient.findAll( DbNames::HaDevicesDBName,
                            JSON( Fields::Category << Fields::SecuritySensor << Fields::Subcategory << Fields::Glass ),
                            nullptr );
    if( !glassSensorDevices.empty() && !houseModeArmed ) {
        auto disarmedDefault = json_get_bool( houseMode, Fields::DisarmedDefault );
        if( disarmedDefault ) {
            for( auto& device : glassSensorDevices ) {
                const std::string& deviceId = json_get_str( device, Fields::Id );
                auto updateArmed = JSON( "$set" << JSON( Fields::Armed << false ) );
                m_dbClient.update( DbNames::HaDevicesDBName, JSON( Fields::Id << deviceId ), updateArmed );
            }
        }
    }

    return true;
}

}  // namespace v1_54_house_mode_glass_breakage_sensor
}  // namespace MigrationTool
}  // namespace HubPlatform
