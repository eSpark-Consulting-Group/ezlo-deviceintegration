#include "house_modes_options_migration.h"
#include "resources.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_30_houseModesOptions {

HouseModesOptionsMigration::HouseModesOptionsMigration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool HouseModesOptionsMigration::migrate() {
    VistaCamMotionSensorMigration vistaCamMigration( m_dbClient );
    m_devicesAdded = 0;

    bool camResult = false;
    {
        const auto filterHouseModesHomeNight =
            JSON( "$or" << JSON_ARRAY_( JSON( Fields::id << "1" ), JSON( Fields::id << "3" ) ) );
        auto filterModes = JSON( Fields::disarmedDefault << true << filterHouseModesHomeNight );
        LOGGER_TRACE( "house_modes_options for disarmed devices by default: " + cc::j( filterModes ) );

        auto dbFilterMotionSensors =
            JSON( Fields::type << Types::sensorMotion << Fields::category << Types::securitySensor
                               << Fields::subcategory << Types::motion );

        // Filter out motion sensors of VistaCam cameras only

        camResult = vistaCamMigration.migrateHouseModesDevices(
            Fields::disarmedDevices, "disarmed devices", dbFilterMotionSensors, filterModes );
        LOGGER_TRACE( std::string{ "VistaCam houseModes motion sensor migration: " } +
                      ( camResult ? "Ok" : "failed" ) );
        m_devicesAdded += vistaCamMigration.devicesAdded();
    }

    bool sirenResult = false;
    {
        auto dbFilterSirens = JSON( Fields::type << Types::siren << Fields::category << Types::siren );
        VistaCamMotionSensorMigration sirenMigration( m_dbClient );
        sirenResult = sirenMigration.migrateHouseModesDevices(
            Fields::alarmsOffDevices, "siren devices", dbFilterSirens, Json{} );
        LOGGER_TRACE( std::string{ "Siren houseModes migration: " } + ( sirenResult ? "Ok" : "failed" ) );
        m_devicesAdded += sirenMigration.devicesAdded();
    }

    return true;
}

int HouseModesOptionsMigration::devicesAdded() const {
    return m_devicesAdded;
}

}  // namespace v1_30_houseModesOptions
}  // namespace MigrationTool
}  // namespace HubPlatform