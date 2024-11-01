#include "migration_tool/migration/migration_manager.h"
#include "migration_tool/migration/dummy_migration.h"
#include "migration_tool/migration/v1_0/migration.h"
#include "migration_tool/migration/v1_1/migration1_1.h"
#include "migration_tool/migration/v1_2/migration1_2.h"
#include "migration_tool/migration/v1_3/migration1_3.h"
#include "migration_tool/migration/v1_4/migration1_4.h"
#include "migration_tool/migration/v1_5/migration1_5.h"
#include "migration_tool/migration/v1_6/migration1_6.h"
#include "migration_tool/migration/v1_7/migration1_7.h"
#include "migration_tool/migration/v1_8/migration1_8.h"
#include "migration_tool/migration/v1_9/migration1_9.h"
#include "migration_tool/migration/v1_10/migration1_10.h"
#include "migration_tool/migration/v1_11/migration1_11.h"
#include "migration_tool/migration/v1_12/migration1_12.h"
#include "migration_tool/migration/v1_13/migration1_13.h"
#include "migration_tool/migration/v1_14/migration1_14.h"
#include "migration_tool/migration/v1_15/migration1_15.h"
#include "migration_tool/migration/v1_16/migration1_16.h"
#include "migration_tool/migration/v1_17/migration1_17.h"
#include "migration_tool/migration/v1_18/migration1_18.h"
#include "migration_tool/migration/v1_19/migration1_19.h"
#include "migration_tool/migration/v1_20/migration1_20.h"
#include "migration_tool/migration/v1_21/migration1_21.h"
#include "migration_tool/migration/v1_22/migration1_22.h"
#include "migration_tool/migration/v1_23/migration.h"
#include "migration_tool/migration/v1_24/migration1_24.h"
#include "migration_tool/migration/v1_25/migration1_25.h"
#include "migration_tool/migration/v1_26/migration1_26.h"
#include "migration_tool/migration/v1_27/migration1_27.h"
#include "migration_tool/migration/v1_28/scale_settings_migration.h"
#include "migration_tool/migration/v1_29/siren_housemodes_migration.h"
#include "migration_tool/migration/v1_30_vcam_motion_sensor/house_modes_options_migration.h"
#include "migration_tool/migration/v1_31/gateway_settings_implementation_migration.h"
#include "migration_tool/migration/v1_32_overheat/overheat_migration.h"
#include "migration_tool/migration/v1_33_panic_button/panic_button_migration.h"
#include "migration_tool/migration/v1_34_cameras/cameras_migration.h"
#include "migration_tool/migration/v1_35_plugin_storages/v1_35_plugin_storages_migration.h"
#include "migration_tool/migration/v1_36_add_scene_block_id/add_scene_block_id.h"
#include "migration_tool/migration/v1_37_securityDevices/security_devices_migration.h"
#include "migration_tool/migration/v1_38_network_statistic/network_statistic_migration.h"
#include "migration_tool/migration/v1_39_security_threat/security_threat_migration.h"
#include "migration_tool/migration/v1_40_bypass_devices/bypass_devices_migration.h"
#include "migration_tool/migration/v1_41_time_array_fix_for_date_event/date_event_time_array_fix_migration.h"
#include "migration_tool/migration/v1_42_acqured_plugins/acqured_plugins_fix_migration.h"
#include "migration_tool/migration/v1_43_script_timeout/script_timeout.h"
#include "migration_tool/migration/v1_44_device_parent_room/device_parent_room.h"
#include "migration_tool/migration/v1_45_house_mode_armed/house_mode_armed_migration.h"
// 1.46 skipped by error
#include "migration_tool/migration/v1_47_protect_feature_on_e450/protect_feature_on_e450_migration.h"
#include "migration_tool/migration/v1_48_rename_grop_db_tables/group_db_rename_migration.h"
#include "migration_tool/migration/v1_49_house_mode_entry_delay/house_mode_entry_delay.h"
#include "migration_tool/migration/v1_50_device_group_new_fields/device_group_new_fields.h"
#include "migration_tool/migration/v1_51_vidoo_camera_devices/vidoo_camera_devices_migration.h"
#include "migration_tool/migration/v1_52_house_mode_alarm_delay_update/house_mode_alarm_delay_update.h"
#include "migration_tool/migration/v1_53_zigbee_gateway_settings/zigbee_gateway_settings.h"
#include "migration_tool/migration/v1_54_house_mode_glass_breakage/house_mode_glass_breakage.h"
#include "migration_tool/migration/v1_55_protect_config/protect_config.h"
#include "migration_tool/migration/v1_56_zigbee_devices/zigbee_devices_reachable.h"
#include "migration_tool/data_resetter.h"

#include "database_client/dbqueryparser.h"
#include "json_utils/json_utils.h"
#include "log_utils/ezlo_log.h"
#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace MigrationTool {

MigrationManager::MigrationManager( IDbClient& dbClient, const ProvisionDataUtils::ProvisionData& provisionData )
: m_dbClient( dbClient )
, m_provisionData( provisionData ) {
    registerMigration< DummyMigration >( 0, 0 );
    registerMigration< v1_0::Migration >( 1, 0, m_dbClient );
    registerMigration< v1_1::Migration >( 1, 1, m_dbClient );
    registerMigration< v1_2::Migration >( 1, 2, m_dbClient );
    registerMigration< v1_3::Migration >( 1, 3, m_dbClient );
    registerMigration< v1_4::Migration >( 1, 4, m_dbClient );
    registerMigration< v1_5::Migration >( 1, 5, m_dbClient );
    registerMigration< v1_6::Migration >( 1, 6, m_dbClient );
    registerMigration< v1_7::Migration >( 1, 7, m_dbClient, m_provisionData );
    registerMigration< v1_8::Migration >( 1, 8, m_dbClient );
    registerMigration< v1_9::Migration >( 1, 9, m_dbClient );
    registerMigration< v1_10::Migration >( 1, 10, m_dbClient );
    registerMigration< v1_11::Migration >( 1, 11, m_dbClient );
    registerMigration< v1_12::Migration >( 1, 12, m_dbClient );
    registerMigration< v1_13::Migration >( 1, 13, m_dbClient );
    registerMigration< v1_14::Migration >( 1, 14, m_dbClient );
    registerMigration< v1_15::Migration >( 1, 15, m_dbClient, m_provisionData );
    registerMigration< v1_16::Migration >( 1, 16, m_dbClient );
    registerMigration< v1_17::Migration >( 1, 17, m_dbClient );
    registerMigration< v1_18::Migration >( 1, 18, m_dbClient );
    registerMigration< v1_19::Migration >( 1, 19, m_dbClient );
    registerMigration< v1_20::Migration >( 1, 20, m_dbClient );
    registerMigration< v1_21::Migration >( 1, 21 );
    registerMigration< v1_22::Migration >( 1, 22, m_dbClient );
    registerMigration< v1_23::Migration >( 1, 23, m_dbClient );
    registerMigration< v1_24::Migration >( 1, 24, m_dbClient );
    registerMigration< v1_25::Migration >( 1, 25, m_dbClient );
    registerMigration< v1_26::Migration >( 1, 26, m_dbClient );
    registerMigration< v1_27::Migration >( 1, 27, m_dbClient );
    registerMigration< v1_28::Migration >( 1, 28, m_dbClient );
    registerMigration< v1_29_sirenHouseModes::SirenHouseModesMigration >( 1, 29, m_dbClient );
    registerMigration< v1_30_houseModesOptions::HouseModesOptionsMigration >( 1, 30, m_dbClient );
    registerMigration< v1_31::Migration >( 1, 31, m_dbClient );
    registerMigration< v1_32_overheat::OverheatMigration >( 1, 32, m_dbClient );
    registerMigration< v1_33_panic::PanicButtonMigration >( 1, 33, m_dbClient );
    registerMigration< v1_34_cameras::CamerasMigration >( 1, 34, m_dbClient );
    registerMigration< v1_35_plugin_storages::PluginStoragesMigration >( 1, 35, m_dbClient );
    registerMigration< v1_36::Migration >( 1, 36, m_dbClient );
    registerMigration< v1_37_securityDevices::SecurityDeviceMigration >( 1, 37, m_dbClient );
    registerMigration< v1_38_network_statistic::Migration >( 1, 38, m_dbClient );
    registerMigration< v1_39_security_threat::Migration >( 1, 39, m_dbClient );
    registerMigration< v1_40_bypass_devices::BypassDevicesMigration >( 1, 40, m_dbClient );
    registerMigration< v1_41_date_event_time_array_fix::DateEventTimeFixMigration >( 1, 41, m_dbClient );
    registerMigration< v1_42_acquired_plugins::AcquiredPluginsMigration >( 1, 42, m_dbClient );
    registerMigration< v1_43_script_timeout::ScriptTimeoutMigration >( 1, 43, m_dbClient );
    registerMigration< v1_44_device_parent_room::DeviceParentRoomMigration >( 1, 44, m_dbClient );
    registerMigration< v1_45_house_mode_armed::HouseModeArmedMigration >( 1, 45, m_dbClient );
    registerMigration< v1_47_protect_feature_on_e450::Migration >( 1, 47, m_dbClient );
    registerMigration< v1_48::Migration >( 1, 48, m_dbClient );
    registerMigration< v1_49_house_mode_entry_delay::Migration >( 1, 49, m_dbClient );
    registerMigration< v1_50_device_group_new_fields::Migration >( 1, 50, m_dbClient );
    registerMigration< v1_51_vidoo_camera_devices::Migration >( 1, 51, m_dbClient );
    registerMigration< v1_52_house_mode_alarm_delay_update::Migration >( 1, 52, m_dbClient );
    registerMigration< v1_53_zigbee_gateway_settings::Migration >( 1, 53, m_dbClient );
    registerMigration< v1_54_house_mode_glass_breakage_sensor::Migration >( 1, 54, m_dbClient );
    registerMigration< v1_55_protect_config::Migration >( 1, 55, m_dbClient );
    registerMigration< v1_56_zigbee_devices::Migration >( 1, 56, m_dbClient );
}

template < typename MigrationType, typename... MigrationArgs >
void MigrationManager::registerMigration( DataFormatVersion::ComponentType major,
                                          DataFormatVersion::ComponentType minor,
                                          MigrationArgs&&... migrationArgs ) {
    m_migrations.emplace( std::piecewise_construct,
                          std::forward_as_tuple( major, minor ),
                          std::forward_as_tuple( std::make_unique< MigrationType >(
                              std::forward< MigrationArgs >( migrationArgs )... ) ) );
}

bool MigrationManager::migrateDatabase() {
    Log::info( "Checking if migration is required..." );

    if( databaseIsEmpty() ) {
        Log::info( "Database is empty, nothing to do" );
        setCurrentDataFormatVersion( getCurrentDataFormatVersion() );
        return true;
    }

    DataFormatVersion existingDataFormatVersion = detectExistingDataFormatVersion();

    if( existingDataFormatVersion == getCurrentDataFormatVersion() ) {
        Log::info( "Data format is up-to-date, nothing to do" );
        return true;
    }

    auto migrationIt = m_migrations.find( existingDataFormatVersion );
    if( migrationIt == m_migrations.end() ) {
        Log::error( "Data format " + existingDataFormatVersion.toString() + " is unsupported!" );

        std::string supportedVersions;
        for( auto& supportedMigrationIt : m_migrations ) {
            if( !supportedVersions.empty() )
                supportedVersions += ", ";
            supportedVersions += supportedMigrationIt.first.toString();
        }
        Log::debug( "Supported versions are: " + supportedVersions );

        HubPlatform::MigrationTool::DataResetter dataResetter( m_dbClient );
        dataResetter.resetData( false );
        return false;
    }

    Log::info( "Data format is out-of-date, performing migration..." );

    while( true ) {
        auto& previousVersion = migrationIt->first;

        migrationIt++;

        if( migrationIt == m_migrations.end() )
            break;

        auto& migration = migrationIt->second;
        auto& migrationVersion = migrationIt->first;

        Log::debug( "Migrating data from " + previousVersion.toString() + " to " + migrationVersion.toString() );
        if( !migration->migrate() ) {
            Log::error( "Migration failed!" );
            HubPlatform::MigrationTool::DataResetter dataResetter( m_dbClient );
            dataResetter.resetData( false );
            return false;
        }

        setCurrentDataFormatVersion( migrationVersion );
    }

    Log::info( "Migration done successfully!" );
    return true;
}

DataFormatVersion MigrationManager::detectExistingDataFormatVersion() const {
    DataFormatVersion initialVersion = { 0, 0 };

    auto result = m_dbClient.findOne( DbVersionsDBName, JSON( db::ID_FIELD_NAME << "firmware" ), nullptr );
    if( result.empty() ) {
        return initialVersion;
    }

    auto versionString = json_get_str( result, "version" );
    if( versionString.empty() ) {
        return initialVersion;
    }

    DataFormatVersion version( versionString );
    if( !version.isValid() ) {
        return initialVersion;
    }

    return version;
}

bool MigrationManager::setCurrentDataFormatVersion( const DataFormatVersion& version ) {
    m_dbClient.update( DbVersionsDBName,
                       JSON_EMPTY_OBJ,
                       JSON( db::ID_FIELD_NAME << "firmware"
                                               << "version" << version.toString() ),
                       true,
                       false );
    return true;
}

bool MigrationManager::databaseIsEmpty() const {
    return m_dbClient.getAllNames().empty();
}

DataFormatVersion MigrationManager::getCurrentDataFormatVersion() const {
    return m_migrations.rbegin()->first;
}

}  // namespace MigrationTool
}  // namespace HubPlatform
