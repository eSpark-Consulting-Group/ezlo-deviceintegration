#include "migration_tool/provisioning.h"
#include "migration_tool/data_resetter.h"
#include "migration_tool/migration/v1_2/migration1_2.h"
#include "migration_tool/migration/v1_3/migration1_3.h"
#include "migration_tool/migration/v1_4/migration1_4.h"
#include "migration_tool/migration/v1_5/migration1_5.h"
#include "migration_tool/migration/v1_6/migration1_6.h"
#include "migration_tool/migration/v1_7/migration1_7.h"
#include "migration_tool/migration/v1_8/migration1_8.h"
#include "migration_tool/migration/v1_12/migration1_12.h"
#include "migration_tool/migration/v1_14/migration1_14.h"
#include "migration_tool/migration/v1_15/migration1_15.h"
#include "migration_tool/migration/v1_16/migration1_16.h"
#include "migration_tool/migration/v1_17/migration1_17.h"
#include "migration_tool/migration/v1_18/migration1_18.h"
#include "migration_tool/migration/v1_19/migration1_19.h"
#include "migration_tool/migration/v1_20/migration1_20.h"
#include "migration_tool/migration/v1_22/migration1_22.h"
#include "migration_tool/migration/v1_23/migration.h"
#include "migration_tool/migration/v1_24/migration1_24.h"
#include "migration_tool/migration/v1_25/migration1_25.h"
#include "migration_tool/migration/v1_28/scale_settings_migration.h"
#include "scenes_manager/tests/resources/scene_scale_resources.h"
#include "config_reader/tests/config_reader_factory.h"
#include "migration_tool/migration/v1_29/siren_housemodes_migration.h"
#include "migration_tool/migration/v1_30_vcam_motion_sensor/house_modes_options_migration.h"
#include "migration_tool/migration/v1_30_vcam_motion_sensor/vistacam_motion_sensor_migration.h"
#include "migration_tool/migration/v1_30_vcam_motion_sensor/resources.h"
#include "migration_tool/migration/v1_31/gateway_settings_implementation_migration.h"
#include "migration_tool/migration/v1_32_overheat/overheat_migration.h"
#include "migration_tool/migration/v1_33_panic_button/panic_button_migration.h"
#include "migration_tool/migration/v1_33_panic_button/resources.h"
#include "migration_tool/migration/v1_34_cameras/cameras_migration.h"
#include "migration_tool/migration/v1_34_cameras/resources.h"
#include "migration_tool/migration/v1_35_plugin_storages/v1_35_plugin_storages_migration.h"
#include "migration_tool/migration/v1_36_add_scene_block_id/add_scene_block_id.h"
#include "migration_tool/migration/v1_38_network_statistic/network_statistic_migration.h"
#include "migration_tool/migration/v1_40_bypass_devices/bypass_devices_migration.h"
#include "migration_tool/migration/v1_41_time_array_fix_for_date_event/date_event_time_array_fix_migration.h"
#include "migration_tool/migration/v1_42_acqured_plugins/acqured_plugins_fix_migration.h"
#include "migration_tool/migration/v1_43_script_timeout/script_timeout.h"
#include "migration_tool/migration/v1_44_device_parent_room/device_parent_room.h"
#include "migration_tool/migration/v1_45_house_mode_armed/house_mode_armed_migration.h"
#include "migration_tool/migration/v1_47_protect_feature_on_e450/protect_feature_on_e450_migration.h"
#include "migration_tool/migration/v1_48_rename_grop_db_tables/group_db_rename_migration.h"
#include "migration_tool/migration/v1_49_house_mode_entry_delay/house_mode_entry_delay.h"
#include "migration_tool/migration/v1_50_device_group_new_fields/device_group_new_fields.h"
#include "migration_tool/migration/v1_51_vidoo_camera_devices/vidoo_camera_devices_migration.h"
#include "migration_tool/migration/v1_52_house_mode_alarm_delay_update/house_mode_alarm_delay_update.h"
#include "migration_tool/tests/v1_41_date_event_time_array_fix_resources.h"
#include "migration_tool/migration/v1_54_house_mode_glass_breakage/house_mode_glass_breakage.h"
#include "migration_tool/migration/v1_55_protect_config/protect_config.h"
#include "migration_tool/migration/v1_56_zigbee_devices/zigbee_devices_reachable.h"
#include "migration_tool/tests/migration_test_fixture.h"
#include "test_utils/temporary_root_folder_wrapper.h"
#include "database_client/idbclient.h"
#include "database_client/dbqueryparser.h"
#include "system_utils/environment.h"
#include "system_utils/filesystem.h"
#include "log_utils/ezlo_log.h"
#include "json_utils/json_utils.h"
#include "config_reader/config_keys.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <iostream>

/*

TESTPLAN
DONE    1. Replace root password
DONE    2. Migrate data to v1.2
DONE    3. Migrate data to v1.3
DONE    4. Migrate data to v1.4
DONE    5. Migrate data to v1.5
DONE    6. Migrate data to v1.6
DONE    7. Migrate data to v1.7
DONE    8. Migrate data to v1.8
DONE    9. Migrate data to 1.12
DONE    10. Migrate data to 1.14
DONE    11. Migrate data to 1.15
DONE    12. Migrate data to 1.16
DONE    13. Migrate data to 1.17
DONE    14. Migrate data to 1.18
DONE    15. Migrate data to 1.19
DONE    16. Migrate data to 1.20
DONE    17. Migrate data to 1.21
DONE    18. Migrate data to 1.23
DONE    19. Migrate data to 1.24
DONE    20. Migrate data to 1.28: scale fields in scene blocks for temperature items
DONE    21. Migrate data to 1.29: sirens to alarmsOffDevices in all houseModes
DONE    22. Migrate data to 1.30: houseModes mode Night, VistaCam motion sensor disarmed
DONE        22.1 house mode mode Home and Night: VistaCam motion sensor disarmed
DONE        22.2 all house modes, siren added to alarmsOffDevices
DONE        22.3 all house modes, disarmedDefault = false: no changes required, custom options are set
DONE    23. Migrate data to 1.31: migration related to gateway settings implementation
DONE    24. Migrate data to 1.32: Overheat devices: subcategory "heat"
DONE    25. Migrate data to 1.33: Panic button devices: subcategory "panic"
DONE    26. Migrate data to 1.34: Add all camera devices to house modes "cameras" collection
DONE    27. Migrate data to 1.35: Plugin storages migration
DONE    28. Migrate data to 1.36: Add scene block id
DONE    29. Migrate data to 1.38: Add network statistic settings table
DONE    30. Migrate data to 1.40: Add "bypassDevices" into house modes
DONE    31. Migrate data to 1.41: fix for "time" parameter of isDate event
DONE        31.1 simple case
DONE        31.2 complicated case with a lot of nesting
DONE    32. Migrate data to 1.42: acquiredPlugins
DONE    33. Migrate data to 1.43: scriptTimeout
DONE    34. Migrate data to 1.44: deviceFollowParent
DONE    35. Migrate data to 1.45: add house mode.armed field
DONE    36. Migrate data to 1.47: Enable "protect" feature on e450 hub
DONE    37. Migrate data to 1.48: group DB rename
DONE    38. Migrate data to 1.49: Add "entryDelay" into house modes
DONE    40. Migrate data to 1.51: Remove parent device from vidoo camera devices
DONE    42. Migrate data to 1.54: Remove glass breakage sensor device from disarmed house mode
DONE    43. Migrate data to 1.56: Zigbee devices reachable states
DONE        43.1 Double reachable state timeout
DONE        43.2 Mark Centralite devices as sleepy
*/

namespace HubPlatform {
namespace UT {

class ProvisioningTestSuite : public testing::Test {
public:
    bool replaceRootPassword( const std::string& filePath, const std::string& replaceWith ) {
        return m_provisioning.replaceRootPassword( filePath, replaceWith );
    }

    std::unique_ptr< std::ofstream > createFile( const std::string& filePath ) {
        return m_provisioning.createFile( filePath );
    }

    const std::string& getTempFolder() const {
        return m_tempFolder.getPath();
    }

private:
    Tests::TemporaryFolderWrapper m_tempFolder;
    Provisioning::Provisioning m_provisioning;
};

TEST_F( ProvisioningTestSuite, replaceRootPassword ) {
    const std::string data = R"(
        sync:*:17212:0:99999:7:::
        games:*:17212:0:99999:7:::
        root:!:17772:0:99999:7:::
        man:*:17212:0:99999:7:::
        lp:*:17212:0:99999:7:::
        )";
    const std::string expectedData = R"(
        sync:*:17212:0:99999:7:::
        games:*:17212:0:99999:7:::
        root:ssssss:17772:0:99999:7:::
        man:*:17212:0:99999:7:::
        lp:*:17212:0:99999:7:::
        )";
    const std::string testFile = getTempFolder() + "/test.txt";
    {
        auto file = createFile( testFile );
        EXPECT_TRUE( file );
        *file << data;
    }
    EXPECT_TRUE( replaceRootPassword( testFile, "ssssss" ) );
    std::ifstream infile( testFile );
    EXPECT_TRUE( infile.good() );
    std::stringstream ss;
    ss << infile.rdbuf();
    EXPECT_EQ( ss.str(), expectedData );
}

class MigrationTest : public MigrationTestFixture {};

TEST_F( MigrationTest, dataResetter ) {
    IDbClient& client = getDBClient();
    client.create( "db.test1" );
    client.create( "db.test2" );
    client.create( "temp.db.test1" );
    client.create( "temp.db.test2" );

    EXPECT_FALSE( client.getAllNames().empty() );

    MigrationTool::DataResetter dataResetter( client );
    dataResetter.resetData();
    EXPECT_TRUE( client.getAllNames().empty() );
}

const std::vector< JsonObject > deviceList = { {
                                                   { "_id", "5cf0dc8c7f000068d223e889" },
                                                   { "batteryPowered", false },
                                                   { "category", "switch" },
                                                   { "deviceTypeId", "634_257_10" },
                                                   { "gatewayId", "5ca480227f00004c03de3f1f" },
                                                   { "info", {} },
                                                   { "name", "Smart Plug" },
                                                   { "parentDeviceId", "" },
                                                   { "reachable", true },
                                                   { "armed", false },
                                                   { "serviceNotification", false },
                                                   { "roomId", "" },
                                                   { "subcategory", "interior_plugin" },
                                                   { "type", "device" },
                                               },
                                               {
                                                   { "_id", "5d6d32710000000a5a0251bf" },
                                                   { "batteryPowered", false },
                                                   { "category", "switch" },
                                                   { "deviceTypeId", "134_3_24" },
                                                   { "gatewayId", "5c44f7a90000000df30069ff" },
                                                   { "info", {} },
                                                   { "name", "Smart Switch" },
                                                   { "parentDeviceId", "" },
                                                   { "reachable", false },
                                                   { "roomId", "" },
                                                   { "subcategory", "interior_plugin" },
                                                   { "type", "switch.outlet" },
                                               } };

TEST_F( MigrationTest, migrationV1_2 ) {
    IDbClient& client = getDBClient();
    std::vector< JsonObject > expected = {
        { { "_id", 1 }, { "name", "Home" }, { "disarmedDefault", true }, { "notifications", { "user1", "user2" } } },
        { { "_id", 2 }, { "name", "Away" }, { "disarmedDefault", true }, { "notifications", { "user1" } } },
        { { "_id", 3 }, { "name", "Night" }, { "disarmedDefault", true }, { "notifications", {} } },
        { { "_id", 4 },
          { "name", "Vacation" },
          { "disarmedDefault", false },
          { "notifications", { "user1", "user2" } } }
    };
    auto HouseModesDBName = "db.firmware.cored.houseModes";
    for( auto& mode : expected ) {
        client.insert( HouseModesDBName, mode );
        if( mode[ "disarmedDefault" ] ) {
            mode[ "notifications" ] = nullptr;
        }
        mode[ "notificationsDefault" ] = nullptr;
    }
    MigrationTool::v1_2::Migration migrationTool( client );
    EXPECT_TRUE( migrationTool.migrate() );
    auto houseModes = client.findAll( HouseModesDBName, JSON_EMPTY_OBJ, nullptr );
    ASSERT_EQ( expected.size(), houseModes.size() );
    for( std::size_t i = 0; i < expected.size(); ++i ) {
        EXPECT_EQ( houseModes[ i ].dump(), expected[ i ].dump() );
    }
}

TEST_F( MigrationTest, migrationV1_3 ) {
    IDbClient& client = getDBClient();
    std::vector< JsonObject > expected = deviceList;
    auto DevicesDBName = "db.firmware.cored.devices";
    for( auto& device : expected ) {
        client.insert( DevicesDBName, device );
        device[ "security" ] = "no";
    }
    MigrationTool::v1_3::Migration migrationTool( client );
    EXPECT_TRUE( migrationTool.migrate() );
    auto devices = client.findAll( DevicesDBName, JSON_EMPTY_OBJ, nullptr );
    ASSERT_EQ( expected.size(), devices.size() );
    for( std::size_t i = 0; i < expected.size(); ++i ) {
        EXPECT_EQ( devices[ i ].dump(), expected[ i ].dump() );
    }
}

TEST_F( MigrationTest, migrationV1_4 ) {
    IDbClient& client = getDBClient();
    std::vector< JsonObject > expected = deviceList;
    auto DevicesDBName = "db.firmware.cored.devices";
    for( auto& device : expected ) {
        client.insert( DevicesDBName, device );
        device[ "ready" ] = true;
    }
    MigrationTool::v1_4::Migration migrationTool( client );
    EXPECT_TRUE( migrationTool.migrate() );
    auto devices = client.findAll( DevicesDBName, JSON_EMPTY_OBJ, nullptr );
    ASSERT_EQ( expected.size(), devices.size() );
    for( std::size_t i = 0; i < expected.size(); ++i ) {
        EXPECT_EQ( devices[ i ].dump(), expected[ i ].dump() );
    }
}

TEST_F( MigrationTest, migrationV1_5 ) {
    IDbClient& client = getDBClient();
    std::vector< JsonObject > expected = deviceList;
    auto DevicesDBName = "db.firmware.cored.devices";
    for( auto& device : expected ) {
        client.insert( DevicesDBName, device );
        device[ "serviceNotification" ] = false;
    }
    MigrationTool::v1_5::Migration migrationTool( client );
    EXPECT_TRUE( migrationTool.migrate() );
    auto devices = client.findAll( DevicesDBName, JSON_EMPTY_OBJ, nullptr );
    ASSERT_EQ( expected.size(), devices.size() );
    for( std::size_t i = 0; i < expected.size(); ++i ) {
        EXPECT_EQ( devices[ i ].dump(), expected[ i ].dump() );
    }
}

TEST_F( MigrationTest, migrationV1_6 ) {
    IDbClient& client = getDBClient();
    std::vector< JsonObject > initValues = { { { "_id", "5cf0dc8c7f000068d223e888" } },
                                             { { "_id", "5cf0dc8c7f000068d223e889" } } };
    for( const auto& gateway : initValues ) {
        client.insert( "db.firmware.cored.gateways", gateway );
    }
    MigrationTool::v1_6::Migration migrationTool( client );
    EXPECT_TRUE( migrationTool.migrate() );

    std::vector< JsonObject > expected = { { { "_id", "5cf0dc8c7f000068d223e888" }, { "status", "not_started" } },
                                           { { "_id", "5cf0dc8c7f000068d223e889" }, { "status", "not_started" } } };

    auto gateways = client.findAll( "db.firmware.cored.gateways", JSON_EMPTY_OBJ, nullptr );
    ASSERT_EQ( expected.size(), gateways.size() );
    for( std::size_t i = 0; i < expected.size(); ++i ) {
        EXPECT_EQ( gateways[ i ].dump(), expected[ i ].dump() );
    }
}

TEST_F( MigrationTest, migrationV1_7 ) {
    IDbClient& db = getDBClient();

    db.insert( "db.firmware.networkd.wifiSettings", JsonObject::parse( R"(
    {
        "_id": "current_settings",
        "key": "secret_password"
    })" ) );

    db.insert( "db.firmware.networkd.wifiSettings", JsonObject::parse( R"(
    {
        "_id": "backup_settings",
        "key": "super_secret_password"
    })" ) );

    db.insert( "db.firmware.uid.items", JsonObject::parse( R"(
    {
        "_id": "5de76e3de4ed900d959dac04",
        "type": "dictionary.userCode",
        "value": {
            "1": {
                "code": "9999",
                "name": "1"
            },
            "44": {
                "code": "4444",
                "name": "44"
            }
        }
    })" ) );

    db.insert( "db.firmware.uid.items", JsonObject::parse( R"(
    {
        "_id": "5de76e3de4ed900d959dac05",
        "type": "dictionary.userCode",
        "value": {}
    })" ) );

    db.insert( "temp.db.firmware.uid.items", JsonObject::parse( R"(
    {
        "_id": "5de76e3de4ed900d959dac05",
        "value": {
            "1": {
                "code": "9999",
                "name": "1"
            },
            "44": {
                "code": "4444",
                "name": "44"
            }
        }
    })" ) );

    ProvisionDataUtils::ProvisionData provisionData;
    provisionData.fillFromJson(
        JsonObject::parse( R"({ "config_version": 1, "id": 70010666, "provision_token": "test" })" ) );
    MigrationTool::v1_7::Migration migrationTool( db, provisionData );
    EXPECT_TRUE( migrationTool.migrate() );

    auto settings = db.findAll( "db.firmware.networkd.wifiSettings", JSON_EMPTY_OBJ, nullptr );
    EXPECT_EQ( settings.size(), 2 );
    EXPECT_EQ( settings[ 0 ][ "key" ].get< std::string >().size(), 64 );
    EXPECT_EQ( settings[ 1 ][ "key" ].get< std::string >().size(), 64 );

    auto staticItems = db.findAll( "db.firmware.uid.items", JSON_EMPTY_OBJ, nullptr );
    EXPECT_EQ( staticItems.size(), 2 );
    EXPECT_EQ( staticItems[ 0 ][ "value" ][ "1" ][ "code" ].get< std::string >().size(), 44 );
    EXPECT_EQ( staticItems[ 0 ][ "value" ][ "44" ][ "code" ].get< std::string >().size(), 44 );
    EXPECT_TRUE( staticItems[ 1 ][ "value" ].empty() );

    auto dynamicItems = db.findAll( "temp.db.firmware.uid.items", JSON_EMPTY_OBJ, nullptr );
    EXPECT_EQ( dynamicItems.size(), 1 );
    EXPECT_EQ( dynamicItems[ 0 ][ "value" ][ "1" ][ "code" ].get< std::string >().size(), 44 );
    EXPECT_EQ( dynamicItems[ 0 ][ "value" ][ "44" ][ "code" ].get< std::string >().size(), 44 );
}

TEST_F( MigrationTest, migrationV1_8 ) {
    const std::string oldCollectionName = "db.firmware.uid.user";
    const std::string oldCertsPath = Environment::getFirmwareRootFolder() + "/certs";

    IDbClient& db = getDBClient();
    db.insert( oldCollectionName,
               JSON( "userId"
                     << "user1"
                     << "offlineToken"
                     << "token1" ) );
    db.insert( oldCollectionName,
               JSON( "userId"
                     << "user2"
                     << "offlineToken"
                     << "token2" ) );

    EXPECT_TRUE( filesystem::createFolder( oldCertsPath ) );
    EXPECT_TRUE( filesystem::isDirectoryExist( oldCertsPath ) );

    MigrationTool::v1_8::Migration migrationTool( db );
    EXPECT_TRUE( migrationTool.migrate() );

    auto allCollections = db.getAllNames();
    EXPECT_TRUE( std::find( allCollections.begin(), allCollections.end(), oldCollectionName ) == allCollections.end() );
    EXPECT_FALSE( filesystem::isDirectoryExist( oldCertsPath ) );

    EXPECT_TRUE( migrationTool.migrate() );
}

TEST_F( MigrationTest, migrationV1_12 ) {
    auto DevicesDBName = "db.firmware.cored.devices";

    IDbClient& client = getDBClient();
    std::vector< JsonObject > expected = deviceList;
    for( auto& device : expected ) {
        client.insert( DevicesDBName, device );
        device[ "status" ] = "idle";
    }

    MigrationTool::v1_12::Migration migrationTool( client );
    EXPECT_TRUE( migrationTool.migrate() );

    auto devices = client.findAll( DevicesDBName, JSON_EMPTY_OBJ, nullptr );
    ASSERT_EQ( expected.size(), devices.size() );
    for( std::size_t i = 0; i < expected.size(); ++i ) {
        EXPECT_EQ( devices[ i ].dump(), expected[ i ].dump() );
    }
}

TEST_F( MigrationTest, migrationV1_14 ) {
    IDbClient& db = getDBClient();

    db.insert( "db.firmware.cored.gateways", JsonObject::parse( R"(
    {
        "_id": "5e6a7f90e4ed900def13cae1",
        "api": {
            "devices": {}
        },
        "name": "cameras"
    })" ) );

    db.insert( "db.firmware.cored.gateways", JsonObject::parse( R"(
    {
        "_id": "5e7b1773e4ed900def13cae2",
        "name": "zwave"
    })" ) );

    MigrationTool::v1_14::Migration migrationTool( db );

    EXPECT_TRUE( migrationTool.migrate() );

    auto gateways = db.findAll( "db.firmware.cored.gateways", JSON_EMPTY_OBJ, nullptr );
    EXPECT_EQ( gateways.size(), 2 );
    EXPECT_EQ( JSON( "_id"
                     << "5e6a7f90e4ed900def13cae1"
                     << "name"
                     << "cameras" )
                   .dump( 4 ),
               gateways[ 0 ].dump( 4 ) );

    EXPECT_EQ( JSON( "_id"
                     << "5e7b1773e4ed900def13cae2"
                     << "name"
                     << "zwave" )
                   .dump( 4 ),
               gateways[ 1 ].dump( 4 ) );
}

TEST_F( MigrationTest, migrationV1_15 ) {
    IDbClient& db = getDBClient();

    db.insert( "db.firmware.uid.credentials", JsonObject::parse( R"(
    {
        "_id": "5eb12aa7e4ed9015d9fdf308",
        "key": "token1",
        "serial": "77777777",
        "uuid": "77777777-7777-7777-7777-777777777777"
    })" ) );

    db.insert( "db.firmware.uid.credentials", JsonObject::parse( R"(
    {
        "_id": "5eb12aa7e4ed9015d9fdf309",
        "key": "token2",
        "serial": "77777777",
        "uuid": "77777777-7777-7777-7777-777777777777"
    })" ) );

    ProvisionDataUtils::ProvisionData provisionData;
    provisionData.fillFromJson(
        JsonObject::parse( R"({ "config_version": 1, "id": 70010666, "provision_token": "test" })" ) );
    MigrationTool::v1_15::Migration migrationTool( db, provisionData );

    EXPECT_TRUE( migrationTool.migrate() );

    auto credentials = db.findAll( "db.firmware.uid.credentials", JSON_EMPTY_OBJ, nullptr );
    EXPECT_EQ( credentials.size(), 2 );
    EXPECT_EQ( credentials[ 0 ][ "key" ].get< std::string >().size(), 44 );
    EXPECT_EQ( credentials[ 1 ][ "key" ].get< std::string >().size(), 44 );
}

TEST_F( MigrationTest, migrationV1_16 ) {
    IDbClient& db = getDBClient();

    db.insert( "db.addons.zigbee.devices", JsonObject::parse( R"(
    {
        "_id": 1
    })" ) );

    db.insert( "db.addons.zigbee.devices", JsonObject::parse( R"(
    {
        "_id": 2,
        "lastUpdateTime": 1516,
        "reachableStateTimeout": 3600
    })" ) );

    db.insert( "db.addons.zigbee.devices", JsonObject::parse( R"(
    {
        "_id": 3
    })" ) );

    MigrationTool::v1_16::Migration migrationTool( db );

    EXPECT_TRUE( migrationTool.migrate() );

    auto nodes = db.findAll( "db.addons.zigbee.devices", JSON_EMPTY_OBJ, nullptr );
    EXPECT_EQ( nodes.size(), 3 );

    EXPECT_FALSE( nodes[ 0 ][ "lastUpdateTime" ].empty() );
    EXPECT_EQ( nodes[ 0 ][ "reachableStateTimeout" ].get< uint32_t >(), 86400 );

    EXPECT_EQ( nodes[ 1 ][ "lastUpdateTime" ].get< uint32_t >(), 1516 );
    EXPECT_EQ( nodes[ 1 ][ "reachableStateTimeout" ].get< uint32_t >(), 3600 );

    EXPECT_FALSE( nodes[ 2 ][ "lastUpdateTime" ].empty() );
    EXPECT_EQ( nodes[ 2 ][ "reachableStateTimeout" ].get< uint32_t >(), 86400 );
}

TEST_F( MigrationTest, migrationV1_17 ) {
    IDbClient& db = getDBClient();

    db.insert( "db.addons.zigbee.devices", JsonObject::parse( R"(
    {
        "_id": 1,
        "isBatteryPowered": true,
        "isReachable": false,
        "lastUpdateTime": 1591173018,
        "manufacturerId": "CentraLite",
        "modelId": "3328-G",
        "reachableStateTimeout": 86400,
        "security": false,
        "endpoints" : [
            {
                "_id": 1,
                "profileId": 1,
                "deviceId": 1,
                "clusters": [
                    {
                        "_id": 1,
                        "endpointId": 1,
                        "name": "on_off",
                        "dataHolder": "server",
                        "attributes": [
                            {
                               "_id": 0,
                                "clusterId": 64513,
                                "formattedValue": "false",
                                "isMfgSpecific": false,
                                "mfgCode": 65535,
                                "name": "unknown",
                                "type": "bool",
                                "value": "false"
                            }
                        ],
                        "commands": [
                            {
                                "_id": 76,
                                "clusterId": 768,
                                "isMfgSpecific": false,
                                "mfgCode": 65535,
                                "name": "Unknown",
                                "type": "received"
                            }
                        ]
                    }
                ]
            }
        ]
    })" ) );

    MigrationTool::v1_17::Migration migrationTool( db );

    EXPECT_TRUE( migrationTool.migrate() );

    auto nodes = db.findAll( "db.addons.zigbee.devices", JSON_EMPTY_OBJ, nullptr );
    EXPECT_EQ( nodes.size(), 1 );

    std::string expected = JsonObject::parse( R"(
        {
            "_id": 1,
            "is_battery_powered": true,
            "is_reachable": false,
            "last_update_time": 1591173018,
            "manufacturer_id": "CentraLite",
            "model_id": "3328-G",
            "reachable_state_timeout": 86400,
            "security": false,
            "endpoints" : [
                {
                    "_id": 1,
                    "profile_id": 1,
                    "device_id": 1,
                    "clusters": [
                        {
                            "_id": 1,
                            "endpoint_id": 1,
                            "name": "on_off",
                            "data_holder": "server",
                            "attributes": [
                                {
                                   "_id": 0,
                                    "cluster_id": 64513,
                                    "formatted_value": "false",
                                    "is_mfg_specific": false,
                                    "mfg_code": 65535,
                                    "name": "unknown",
                                    "type": "bool",
                                    "value": "false"
                                }
                            ],
                            "commands": [
                                {
                                    "_id": 76,
                                    "cluster_id": 768,
                                    "is_mfg_specific": false,
                                    "mfg_code": 65535,
                                    "name": "Unknown",
                                    "type": "received"
                                }
                            ]
                        }
                    ]
                }
            ]
        })" )
                               .dump();

    EXPECT_EQ( std::string( nodes[ 0 ].dump() ), expected );
}

TEST_F( MigrationTest, migrationV1_18 ) {
    IDbClient& db = getDBClient();

    db.insert( "db.addons.zigbee.devices", JsonObject::parse( R"(
    {
        "_id": 1,
        "manufacturer_id": "",
        "model_id": ""
    })" ) );

    db.insert( "db.addons.zigbee.devices", JsonObject::parse( R"(
    {
        "_id": 2,
        "manufacturer_id": "test",
        "model_id": ""
    })" ) );

    db.insert( "db.addons.zigbee.devices", JsonObject::parse( R"(
    {
        "_id": 3,
        "manufacturer_id": "",
        "model_id": "test"
    })" ) );

    db.insert( "db.addons.zigbee.devices", JsonObject::parse( R"(
    {
        "_id": 4,
        "manufacturer_id": "test",
        "model_id": "test"
    })" ) );

    MigrationTool::v1_18::Migration migrationTool( db );

    EXPECT_TRUE( migrationTool.migrate() );

    auto nodes = db.findAll( "db.addons.zigbee.devices", JSON_EMPTY_OBJ, nullptr );
    EXPECT_EQ( nodes.size(), 4 );

    EXPECT_EQ( nodes[ 0 ][ "manufacturer_id" ], "Unknown" );
    EXPECT_EQ( nodes[ 0 ][ "model_id" ], "Unknown" );

    EXPECT_EQ( nodes[ 1 ][ "manufacturer_id" ], "test" );
    EXPECT_EQ( nodes[ 1 ][ "model_id" ], "Unknown" );

    EXPECT_EQ( nodes[ 2 ][ "manufacturer_id" ], "Unknown" );
    EXPECT_EQ( nodes[ 2 ][ "model_id" ], "test" );

    EXPECT_EQ( nodes[ 3 ][ "manufacturer_id" ], "test" );
    EXPECT_EQ( nodes[ 3 ][ "model_id" ], "test" );
}

TEST_F( MigrationTest, migrationV1_19 ) {
    IDbClient& client = getDBClient();
    std::vector< JsonObject > initValues = { { { "_id", "5cf0dc8c7f000068d223e888" }, { "manualDeviceAdding", "no" } },
                                             { { "_id", "5cf0dc8c7f000068d223e889" } } };
    for( const auto& gateway : initValues ) {
        client.insert( "db.firmware.cored.gateways", gateway );
    }
    MigrationTool::v1_19::Migration migrationTool( client );
    EXPECT_TRUE( migrationTool.migrate() );

    std::vector< JsonObject > expected = { { { "_id", "5cf0dc8c7f000068d223e888" }, { "manualDeviceAdding", "no" } },
                                           { { "_id", "5cf0dc8c7f000068d223e889" },
                                             { "manualDeviceAdding", "wizard" } } };

    auto gateways = client.findAll( "db.firmware.cored.gateways", JSON_EMPTY_OBJ, nullptr );
    ASSERT_EQ( expected.size(), gateways.size() );
    for( std::size_t i = 0; i < expected.size(); ++i ) {
        EXPECT_EQ( gateways[ i ].dump(), expected[ i ].dump() );
    }
}

TEST_F( MigrationTest, migrationV1_20 ) {
    auto& client = getDBClient();

    client.insert( MigrationTool::v1_20::m_dataTableName, JSON_EMPTY_OBJ );
    EXPECT_TRUE( client.exist( MigrationTool::v1_20::m_dataTableName ) );

    MigrationTool::v1_20::Migration migrationTool( client );
    EXPECT_TRUE( migrationTool.migrate() );

    EXPECT_FALSE( client.exist( MigrationTool::v1_20::m_dataTableName ) );
}

TEST_F( MigrationTest, migrationV1_21 ) {
    // Dummy migration
}

TEST_F( MigrationTest, migrationV1_22 ) {
    const auto dbNames = { "db.firmware.cored.gateways", HaItemsDBName, HaDevicesDBName };

    auto& client = getDBClient();

    for( auto dbName : dbNames ) {
        client.insert( dbName,
                       JSON( "_id"
                             << "id1"
                             << "value" << 1 ) );
        client.insert( dbName,
                       JSON( "_id"
                             << "id2"
                             << "value" << 2 ) );

        client.update( dbName,
                       JSON( "_id"
                             << "id2" ),
                       []( Json& data ) { data[ "_id" ] = "fixed_id"; } );
    }

    MigrationTool::v1_22::Migration migrationTool( client );

    EXPECT_TRUE( migrationTool.migrate() );

    for( auto dbName : dbNames ) {
        EXPECT_EQ( client
                       .findOne( dbName,
                                 JSON( "_id"
                                       << "id1" ),
                                 nullptr )
                       .dump(),
                   JSON( "_id"
                         << "id1"
                         << "value" << 1 )
                       .dump() );
        EXPECT_EQ( client
                       .findOne( dbName,
                                 JSON( "_id"
                                       << "fixed_id" ),
                                 nullptr )
                       .dump(),
                   JSON( "_id"
                         << "fixed_id"
                         << "value" << 2 )
                       .dump() );
        EXPECT_TRUE( client
                         .findOne( dbName,
                                   JSON( "_id"
                                         << "id2" ),
                                   nullptr )
                         .empty() );
    }
}

TEST_F( MigrationTest, migrationV1_23 ) {
    auto& client = getDBClient();

    auto oldCameraStreamDb = "Cameras.cameras";
    auto newCameraStreamDb = "db.addons.cameras.streams";

    client.insert( oldCameraStreamDb,
                   JSON( "stream"
                         << "id1" ) );
    client.insert( oldCameraStreamDb,
                   JSON( "stream"
                         << "id2" ) );

    auto dbTunnels = "db.firmware.networkd.cloud.ssh.tunnels";
    auto dbItems = "db.firmware.uid.items";

    client.insert( dbItems,
                   JSON( "_id"
                         << "5f9a8f2d0000003baa33cb81"
                         << "name"
                         << "camera_stream"
                         << "value"
                         << JSON( "localRtspUrl"
                                  << "rtsp://192.168.33.102:5551/5f9a8f2d0000003baa33cb81" ) ) );
    client.insert( dbItems,
                   JSON( "_id"
                         << "5f9a8f2d0000003baa33cb82"
                         << "name"
                         << "camera_stream"
                         << "value"
                         << JSON( "localRtspUrl"
                                  << "rtsp://192.168.33.102:5552/5f9a8f2d0000003baa33cb82" ) ) );

    MigrationTool::v1_23::Migration migrationTool( client );

    EXPECT_TRUE( migrationTool.migrate() );

    EXPECT_FALSE( client.exist( oldCameraStreamDb ) );
    EXPECT_TRUE( client.exist( newCameraStreamDb ) );

    auto streams = client.findAll( newCameraStreamDb,
                                   JSON( "stream"
                                         << "id1" ),
                                   nullptr );
    EXPECT_EQ( streams.size(), 1 );
    streams = client.findAll( newCameraStreamDb,
                              JSON( "stream"
                                    << "id2" ),
                              nullptr );
    EXPECT_EQ( streams.size(), 1 );

    auto tunnel = client.findOne( dbTunnels,
                                  JSON( "identity"
                                        << "5f9a8f2d0000003baa33cb81" ),
                                  nullptr );
    EXPECT_EQ( tunnel[ "localPort" ], 5551 );
    EXPECT_EQ( tunnel[ "method" ], "cloud.tunnel_camera_coordinator" );
    EXPECT_EQ( tunnel.count( "token" ), 1 );
    EXPECT_EQ( tunnel.count( "tokenExpirationDate" ), 1 );
    EXPECT_EQ( tunnel.count( "tunnelLink" ), 1 );
    EXPECT_EQ( tunnel.count( "tunnelPort" ), 1 );

    tunnel = client.findOne( dbTunnels,
                             JSON( "identity"
                                   << "5f9a8f2d0000003baa33cb82" ),
                             nullptr );
    EXPECT_EQ( tunnel[ "localPort" ], 5552 );
    EXPECT_EQ( tunnel[ "method" ], "cloud.tunnel_camera_coordinator" );
    EXPECT_EQ( tunnel.count( "token" ), 1 );
    EXPECT_EQ( tunnel.count( "tokenExpirationDate" ), 1 );
    EXPECT_EQ( tunnel.count( "tunnelLink" ), 1 );
    EXPECT_EQ( tunnel.count( "tunnelPort" ), 1 );
}

const std::vector< JsonObject > provisioningList = {
    { { "_id", "2224506993" }, { "dsk", "32313433363538373139333235343736" }, { "name", "test_name1" } },
    { { "_id", "2224506994" },
      { "dsk", "32313433363538373139333235343737" },
      { "name", "test_name2" },
      { "inclusionSetting", "passive" } }
};

TEST_F( MigrationTest, migrationV1_24 ) {
    auto ProvisioningListDBName = "db.addons.zwave.provisioning";

    IDbClient& client = getDBClient();
    for( const auto& entry : provisioningList ) {
        client.insert( ProvisioningListDBName, entry );
    }

    MigrationTool::v1_24::Migration migrationTool( client );
    EXPECT_TRUE( migrationTool.migrate() );

    std::vector< JsonObject > expected = provisioningList;
    expected[ 0 ][ "inclusionSetting" ] = "pending";

    auto entries = client.findAll( ProvisioningListDBName, JSON_EMPTY_OBJ, nullptr );
    ASSERT_EQ( expected.size(), entries.size() );

    for( std::size_t i = 0; i < expected.size(); ++i ) {
        EXPECT_EQ( entries[ i ].dump(), expected[ i ].dump() );
    }
}

TEST_F( MigrationTest, migrationV1_28_scale_settings_modifier ) {
    Log::setLocalLogLevel( Log::PriorityLevel::TRACE );
    Json testScene = SceneScale::testScene;

    auto itemsDb = "db.firmware.uid.items";

    IDbClient& client = getDBClient();
    Json itemSetPointHeating = SceneScale::itemSetPointHeating;
    Json itemThermostatMode = SceneScale::itemThermostatMode;
    Json itemThermostatSetPoint = SceneScale::itemThermostatSetPoint;

    // Create items
    client.insert( itemsDb, itemSetPointHeating );
    client.insert( itemsDb, itemThermostatMode );
    client.insert( itemsDb, itemThermostatSetPoint );

    // create scenes
    auto scenesDb = "db.firmware.scenes_manager.rules";
    client.insert( scenesDb, testScene );

    MigrationTool::v1_28::Migration migrationTool( client );
    try {
        EXPECT_TRUE( migrationTool.migrate() );
        EXPECT_EQ( migrationTool.getScenesUpdatedCount(), 1 ) << "One test scene should be updated";
    }
    catch( const std::exception& error ) {
        FAIL() << "Exception thrown: " << error.what();
    }

    Json all = client.findAll( scenesDb, Json{ { "_id", testScene[ "_id" ] } }, nullptr );
    auto dump = all.dump();
    ASSERT_NE( dump.find( "celsius" ), std::string::npos ) << "Should found the string 'celsius in a scene'";

    SUCCEED();
}

namespace Resources {

Json sirenDevice = R"({
        "_id": "sirenDevice1",
        "category": "siren",
        "deviceTypeId": "e550_siren",
        "name": "Controller Siren",
        "subcategory": "",
        "type": "siren"
    }
)"_json;

Json houseModeHome = R"({
    "_id": "1",
    "name": "Home",
    "armedCategories": [],
    "alarmCategories": [{ "category": "siren" }],
    "alarmsOffDevices": [],
    "disarmedDevices": [],
    "disarmedDefault": true,
    "armed": false
})"_json;

Json houseModeAway = R"({
    "_id": "2",
    "name": "Away",
    "armedCategories": [],
    "alarmCategories": [{ "category": "siren" }],
    "alarmsOffDevices": [],
    "disarmedDevices": [],
    "disarmedDefault": true,
    "armed": true
})"_json;

Json houseModeNight = R"({
    "_id": "3",
    "name": "Night",
    "armedCategories": [],
    "alarmCategories": [{ "category": "siren" }],
    "alarmsOffDevices": [],
    "disarmedDevices": [],
    "disarmedDefault": true
})"_json;

Json houseModeVacation = R"({
    "_id": "4",
    "name": "Vacation",
    "armedCategories": [],
    "alarmCategories": [{ "category": "siren" }],
    "alarmsOffDevices": [],
    "disarmedDevices": [],
    "disarmedDefault": true
})"_json;

Json vistaCam = R"({
    "_id": "vistaCamDevice1",
    "batteryPowered": false,
    "category": "camera",
    "deviceTypeId": "ezviz_vistacam702",
    "info": {
        "firmware": "V5.2.7 build 200717",
        "mac": "98:8b:0a:f4:88:a4",
        "manufacturer": "Ezlo",
        "model": "VistaCam702",
        "serial_number": "85001569",
        "uuid": "b90d7ee0-f4b8-11e9-a1a5-fbc69fd9ba07"
    },
    "name": "VistaCam702 85001569",
    "parentDeviceId": "",
    "persistent": false,
    "reachable": true,
    "ready": true,
    "roomId": "",
    "security": "no",
    "status": "idle",
    "subcategory": "indoor_cam",
    "type": "ezviz_camera"
})"_json;

Json vistaCamMotionSensor = R"({
    "_id": "motionSensorDevice1",
    "armed": false,
    "batteryPowered": false,
    "category": "security_sensor",
    "deviceTypeId": "ezviz_vistacam702_motion_sensor",
    "gatewayId": "5fcf5e4a2231401776b08405",
    "info": {
        "manufacturer": "Ezlo", "model": "VistaCam702"
    },
    "name": "VistaCam702 XXXXXXXX Motion Sensor",
    "parentDeviceId": "vistaCamDevice1",
    "persistent": false,
    "reachable": true,
    "ready": true,
    "roomId": "",
    "security": "no",
    "status": "idle",
    "subcategory": "motion",
    "type": "sensor.motion"
})"_json;

};  // namespace Resources

TEST_F( MigrationTest, migrationV1_29_sirenHouseModes ) {
    Log::setLocalLogLevel( Log::PriorityLevel::TRACE );

    const char* devicesDb = "db.firmware.cored.devices";
    const char* houseModesDb = "db.firmware.cored.houseModes";
    const char* coredSettingsDb = "db.firmware.cored.settings";

    IDbClient& client = getDBClient();

    // Create siren device
    client.insert( devicesDb, Resources::sirenDevice );

    // create houseModes
    const int expectedInserts = 2;
    ASSERT_NO_THROW( client.insert( houseModesDb, Resources::houseModeHome ) );
    ASSERT_NO_THROW( client.insert( houseModesDb, Resources::houseModeAway ) );

    Json houseModesSettings = JSON( "_id"
                                    << "houseMode"
                                    << "alarmDelay" << 0 << "houseMode"
                                    << "1"
                                    << "switchToDelay" << 30 );
    Json configSettings = JSON( "settings" << houseModesSettings );
    // create cored houseModes settings
    ASSERT_NO_THROW( client.insert( coredSettingsDb, houseModesSettings ) );

    class ConfigFactoryImpl : public UT::ConfigReaderFactory {
    public:
        using UT::ConfigReaderFactory::createConfigReader;
    };

    ConfigReader configReader = ConfigFactoryImpl::createConfigReader( [&]( const std::string& source ) {
        LOGGER_TRACE_RAW( "ConfigReader fake factory: " + cc::j( source ) + ", value: " + cc::j( configSettings ) );
        return configSettings.dump();
    } );

    MigrationTool::v1_29_sirenHouseModes::SirenHouseModesMigration migrationTool( client, &configReader );
    try {
        EXPECT_TRUE( migrationTool.migrate() );
        EXPECT_EQ( migrationTool.sirensAdded(), expectedInserts ) << "Sirens added to houseModes";
    }
    catch( const std::exception& error ) {
        FAIL() << "Exception thrown: " << error.what();
    }

    auto resultSet = client.findAll( houseModesDb, {}, nullptr );
    for( const auto& mode : resultSet ) {
        LOGGER_TRACE_RAW( "result: " + cc::j( mode ) );
        Json offDevices;
        ASSERT_TRUE( json_get_arr( mode, "alarmsOffDevices", offDevices ) );
        ASSERT_TRUE( offDevices.is_array() && !offDevices.empty() );
        ASSERT_EQ( offDevices[ 0 ], "sirenDevice1" );
    }

    // cored settings check
    auto houseModeSetting = client.findOne( coredSettingsDb,
                                            JSON( "_id"
                                                  << "houseMode" ),
                                            nullptr );
    LOGGER_TRACE_RAW( "houseModeSetting: " + cc::j( houseModeSetting ) );
    ASSERT_FALSE( houseModeSetting.empty() );
    ASSERT_EQ( json_get_int( houseModeSetting, "alarmDelay" ), 0 ) << "Should change delay from 30 to 0";

    SUCCEED();
}

namespace {
namespace Fields {
const char* devices = "devices";
const char* disarmedDevices = "disarmedDevices";
const char* alarmsOffDevices = "alarmsOffDevices";
const char* disarmedDefault = "disarmedDefault";

// device fields
const char* const id = "_id";
const char* const deviceTypeId = "deviceTypeId";
}  // namespace Fields
}  // namespace

class HouseModesOptionsMigrationTest : public MigrationTestFixture {};

TEST_F( HouseModesOptionsMigrationTest, migrationV1_30_houseModesOptions_disarmedDevices ) /* 22.1 */ {
    Log::setLocalLogLevel( Log::PriorityLevel::TRACE );

    const char* devicesDb = "db.firmware.cored.devices";
    const char* houseModesDb = "db.firmware.cored.houseModes";

    IDbClient& client = getDBClient();

    // Create devices: vistaCam, motion sensor child device
    client.insert( devicesDb, Resources::vistaCam );
    client.insert( devicesDb, Resources::vistaCamMotionSensor );
    std::vector< Json > testDevices = { Resources::vistaCamMotionSensor };
    {
        Json vistaCam1200sensor = Resources::vistaCamMotionSensor;
        vistaCam1200sensor[ Fields::id ] = "motionSensorDevice2";
        vistaCam1200sensor[ Fields::deviceTypeId ] = "ezviz_vistacam1200_motion_sensor";
        client.insert( devicesDb, vistaCam1200sensor );

        testDevices.push_back( vistaCam1200sensor );
    }

    // create houseModes
    const int numberOfSensors = 2;
    const int numberOfModes = 2;
    const int expectedInserts = numberOfSensors * numberOfModes;  // for motion sensor: Home, Night * 2 sensors
    ASSERT_NO_THROW( client.insert( houseModesDb, Resources::houseModeHome ) );
    ASSERT_NO_THROW( client.insert( houseModesDb, Resources::houseModeAway ) );
    ASSERT_NO_THROW( client.insert( houseModesDb, Resources::houseModeNight ) );
    ASSERT_NO_THROW( client.insert( houseModesDb, Resources::houseModeVacation ) );

    const Json houseModesAffected = JSON( "1" << true << "3" << true );

    MigrationTool::v1_30_houseModesOptions::HouseModesOptionsMigration migrationTool( client );
    try {
        EXPECT_TRUE( migrationTool.migrate() );
        EXPECT_EQ( migrationTool.devicesAdded(), expectedInserts )
            << "Devices added to the Night houseMode as disarmed";
    }
    catch( const std::exception& error ) {
        FAIL() << "Exception thrown: " << error.what();
    }

    auto resultSet = client.findAll( houseModesDb, {}, nullptr );

    std::for_each( begin( testDevices ), end( testDevices ), [&]( const Json& device ) {
        for( const auto& mode : resultSet ) {
            if( json_contains( houseModesAffected, mode[ Fields::id ] ) ) {
                LOGGER_TRACE_RAW( "result: " + cc::j( mode ) );

                Json disarmedDevices;
                ASSERT_TRUE( json_get_arr( mode, Fields::disarmedDevices, disarmedDevices ) );
                ASSERT_TRUE( disarmedDevices.is_array() && !disarmedDevices.empty() );

                bool isFound = false;
                for( auto deviceId : disarmedDevices ) {
                    if( deviceId == json_get_str( device, Fields::id ) ) {
                        isFound = true;
                        break;
                    }
                }

                ASSERT_TRUE( isFound ) << "Should have the device id in disarmedDevices[]";
            }
        }
    } );

    SUCCEED();
}

TEST_F( HouseModesOptionsMigrationTest, migrationV1_30_houseModesOptions_alarmsOffDevices ) /* 22.2 */ {
    Log::setLocalLogLevel( Log::PriorityLevel::TRACE );

    const char* devicesDb = "db.firmware.cored.devices";
    const char* houseModesDb = "db.firmware.cored.houseModes";

    IDbClient& client = getDBClient();

    // Create devices: vistaCam, motion sensor child device
    client.insert( devicesDb, Resources::sirenDevice );

    // create houseModes
    const int expectedInserts = 4;  // for siren: all 4 modes
    ASSERT_NO_THROW( client.insert( houseModesDb, Resources::houseModeHome ) );
    ASSERT_NO_THROW( client.insert( houseModesDb, Resources::houseModeAway ) );
    ASSERT_NO_THROW( client.insert( houseModesDb, Resources::houseModeNight ) );
    ASSERT_NO_THROW( client.insert( houseModesDb, Resources::houseModeVacation ) );

    MigrationTool::v1_30_houseModesOptions::HouseModesOptionsMigration migrationTool( client );
    try {
        EXPECT_TRUE( migrationTool.migrate() );
        EXPECT_EQ( migrationTool.devicesAdded(), expectedInserts ) << "Devices added to all modes alarmsOff";
    }
    catch( const std::exception& error ) {
        FAIL() << "Exception thrown: " << error.what();
    }

    auto resultSet = client.findAll( houseModesDb, {}, nullptr );
    for( const auto& mode : resultSet ) {
        auto modeName = json_get_str( mode, "name" );

        Json devices;
        auto collectionName = Fields::alarmsOffDevices;
        ASSERT_TRUE( json_get_arr( mode, collectionName, devices ) );

        LOGGER_TRACE_RAW( "house mode: " + cc::j( mode ) );
        ASSERT_TRUE( devices.is_array() && !devices.empty() ) << "Should be filled with siren device id";
        ASSERT_EQ( devices[ 0 ], Resources::sirenDevice.at( Fields::id ) );
    }

    SUCCEED();
}

TEST_F( HouseModesOptionsMigrationTest, migrationV1_30_houseModesOptions_disarmedDefaultIsFalse ) /* 22.3 */ {
    const char* devicesDb = "db.firmware.cored.devices";
    const char* houseModesDb = "db.firmware.cored.houseModes";

    IDbClient& client = getDBClient();

    // Create devices: vistaCam, motion sensor child device
    client.insert( devicesDb, Resources::vistaCam );
    client.insert( devicesDb, Resources::vistaCamMotionSensor );

    // create houseModes
    const int expectedInserts = 0;  // for motion sensor: Home, Night
    Json homeModeChanged = Resources::houseModeHome;
    homeModeChanged[ "disarmedDefault" ] = false;
    ASSERT_NO_THROW( client.insert( houseModesDb, homeModeChanged ) );

    MigrationTool::v1_30_houseModesOptions::HouseModesOptionsMigration migrationTool( client );
    try {
        EXPECT_TRUE( migrationTool.migrate() );
        EXPECT_EQ( migrationTool.devicesAdded(), expectedInserts )
            << "Devices added to the Night houseMode as disarmed";
    }
    catch( const std::exception& error ) {
        FAIL() << "Exception thrown: " << error.what();
    }

    auto resultSet = client.findAll( houseModesDb, {}, nullptr );
    for( const auto& mode : resultSet ) {
        LOGGER_TRACE_RAW( "result: " + cc::j( mode ) );

        Json disarmedDevices;
        ASSERT_TRUE( json_get_arr( mode, Fields::disarmedDevices, disarmedDevices ) );
        ASSERT_TRUE( disarmedDevices.is_array() && disarmedDevices.empty() )
            << "Should be unchanged for mode.disarmedDefault == false";
    }

    SUCCEED();
}

TEST_F( MigrationTest, migrationV1_31_gatewaySettingsImplementation ) /* 23 */ {
    auto& client = getDBClient();

    client.insert( MigrationTool::v1_31::gatewaysTableName, JSON( db::ID_FIELD_NAME << "1" ) );
    client.insert( MigrationTool::v1_31::gatewaysTableName, JSON( db::ID_FIELD_NAME << "2" ) );

    {
        const auto gates = client.findAll( MigrationTool::v1_31::gatewaysTableName, JSON_EMPTY_OBJ, nullptr );
        for( const auto& gate : gates ) {
            ASSERT_FALSE( json_contains( gate, MigrationTool::v1_31::settingsFieldName ) );
        }
    }

    MigrationTool::v1_31::Migration migrationTool( client );
    ASSERT_TRUE( migrationTool.migrate() );

    {
        const auto gates = client.findAll( MigrationTool::v1_31::gatewaysTableName, JSON_EMPTY_OBJ, nullptr );
        for( const auto& gate : gates ) {
            ASSERT_TRUE( json_contains( gate, MigrationTool::v1_31::settingsFieldName ) );
            EXPECT_EQ( json_get_int( gate, MigrationTool::v1_31::settingsFieldName, -1 ), 0 );
        }
    }
}

TEST_F( HouseModesOptionsMigrationTest, migrationV1_32_overheat ) /* 24 */ {
    const char* devicesDb = "db.firmware.cored.devices";
    const char* houseModesDb = "db.firmware.cored.houseModes";

    IDbClient& client = getDBClient();

    Json overheatChildDevice = Resources::vistaCamMotionSensor;
    overheatChildDevice[ "type" ] = "sensor";
    overheatChildDevice[ "category" ] = "security_sensor";
    overheatChildDevice[ "subcategory" ] = "heat";

    // Create devices: vistaCam, motion sensor child device
    client.insert( devicesDb, Resources::vistaCam );
    client.insert( devicesDb, overheatChildDevice );

    // create houseModes
    const int expectedInserts = 4;
    auto modes = {
        Resources::houseModeHome, Resources::houseModeAway, Resources::houseModeNight, Resources::houseModeVacation
    };
    for( auto mode : modes ) {
        Json& armedCategories = mode[ "armedCategories" ];
        armedCategories.push_back( JSON( "category"
                                         << "security_sensor"
                                         << "subcategories" << JSON_ARRAY_( "co" ) ) );

        ASSERT_NO_THROW( client.insert( houseModesDb, mode ) );
    }

    MigrationTool::v1_32_overheat::OverheatMigration migrationTool( client );
    try {
        EXPECT_TRUE( migrationTool.migrate() );
        EXPECT_EQ( migrationTool.recordsModified(), expectedInserts ) << "Records modified while migrating";
    }
    catch( const std::exception& error ) {
        FAIL() << "Exception thrown: " << error.what();
    }

    SUCCEED();
}

TEST_F( HouseModesOptionsMigrationTest, migrationV1_33_panic_button ) /* 25 */ {
    Log::setLocalLogLevel( Log::PriorityLevel::TRACE );
    namespace Fields = HubPlatform::MigrationTool::v1_33_panic::Fields;
    namespace Tables = HubPlatform::MigrationTool::v1_33_panic::Tables;
    namespace Values = HubPlatform::MigrationTool::v1_33_panic::Values;

    const char* houseModesDb = Tables::houseModes;
    const char* devicesDb = Tables::devices;

    IDbClient& client = getDBClient();

    Json device = Resources::vistaCamMotionSensor;
    device[ Fields::type ] = Values::controllerPanic;
    device[ Fields::category ] = Values::sceneController;
    device[ Fields::subcategory ] = Values::panic;
    device[ "deviceTypeId" ] = "";
    device[ "parentDeviceId" ] = "";
    device[ "name" ] = "test panic button";

    client.insert( devicesDb, device );

    // create houseModes
    const int expectedInserts = 4;
    auto modes = {
        Resources::houseModeHome, Resources::houseModeAway, Resources::houseModeNight, Resources::houseModeVacation
    };
    for( auto mode : modes ) {
        Json& armedCategories = mode[ "armedCategories" ];
        armedCategories.push_back( JSON( "category"
                                         << "security_sensor"
                                         << "subcategories" << JSON_ARRAY_( "co" ) ) );
        ASSERT_NO_THROW( client.insert( houseModesDb, mode ) );
    }

    MigrationTool::v1_33_panic::PanicButtonMigration migrationTool( client );
    try {
        EXPECT_TRUE( migrationTool.migrate() );
        EXPECT_EQ( migrationTool.recordsModified(), expectedInserts ) << "Records modified while migrating";

        auto modesFetched = getDBClient().findAll( Tables::houseModes, JSON_EMPTY_OBJ, nullptr );
        EXPECT_EQ( modesFetched.size(), 4 );

        for( const auto& mode : modesFetched ) {
            JsonArray armedCategories;
            ASSERT_TRUE( json_get( mode, "armedCategories", armedCategories ) ) << "Mode: " << mode;

            auto sceneControllerIt =
                std::find_if( begin( armedCategories ), end( armedCategories ), []( const JsonObject& json ) {
                    return json_contains( json, Fields::category ) &&
                           json[ Fields::category ] == Values::sceneController;
                } );

            auto sceneControllerOptions = *sceneControllerIt;
            EXPECT_TRUE( json_contains( sceneControllerOptions, Fields::subcategories ) ) << "Mode: " << mode;

            JsonArray subcategories;
            EXPECT_TRUE( json_get_arr( sceneControllerOptions, Fields::subcategories, subcategories ) )
                << "Mode: " << mode;
            auto panicFound = std::any_of( begin( subcategories ), end( subcategories ), []( const Json& elem ) {
                return json_get_str( elem ) == Values::panic;
            } );
            EXPECT_TRUE( panicFound ) << "Should have 'panic' in subcategories array: " << mode;
        }
    }
    catch( const std::exception& error ) {
        FAIL() << "Exception thrown: " << error.what();
    }
}

TEST_F( HouseModesOptionsMigrationTest, migrationV1_34_cameras ) /* 26 */ {
    Log::setLocalLogLevel( Log::PriorityLevel::TRACE );
    namespace Fields = HubPlatform::MigrationTool::v1_34_cameras::Fields;
    namespace Tables = HubPlatform::MigrationTool::v1_34_cameras::Tables;
    namespace Values = HubPlatform::MigrationTool::v1_34_cameras::Values;

    const char* houseModesCamerasDb = Tables::HouseModesCamerasDb;
    const char* devicesDb = Tables::DevicesDb;
    const char* itemsDb = Tables::ItemsDb;

    IDbClient& client = getDBClient();

    // create a camera device
    // create a make_recording item in it
    Json device = Resources::vistaCamMotionSensor;
    device[ Fields::id ] = "camera_id1";
    device[ Fields::type ] = Values::cameraIndoor;
    device[ Fields::category ] = Values::camera;
    device[ Fields::subcategory ] = Values::cameraIndoor;  // ?
    device[ "deviceTypeId" ] = "test_camera";
    device[ "parentDeviceId" ] = "";
    device[ Fields::name ] = "Generic camera device";

    client.insert( devicesDb, device );

    device[ Fields::id ] = "camera_id2";
    client.insert( devicesDb, device );

    Json item = { { Fields::id, "make_recording_id1" },
                  { Fields::name, "make_recording" },
                  { Fields::hasSetter, true },
                  { Fields::hasGetter, false },
                  { Fields::show, true },
                  { Fields::value, 0 },
                  { Fields::type, "int" },
                  { Fields::deviceId, "camera_id1" },
                  { Fields::enumField, JSON_EMPTY_ARR } };
    client.insert( itemsDb, item );

    item[ Fields::id ] = "make_recording_id2";
    item[ Fields::deviceId ] = "camera_id2";
    item[ Fields::hasSetter ] = false;  // not a camera for house modes, no hasSetter
    client.insert( itemsDb, item );

    const int expectedInserts = 1;

    MigrationTool::v1_34_cameras::CamerasMigration migrationTool( client );
    try {
        EXPECT_TRUE( migrationTool.migrate() );
        EXPECT_EQ( migrationTool.recordsModified(), expectedInserts ) << "Records modified while migrating";

        auto resultSet = client.findAll( Tables::HouseModesCamerasDb, JSON_EMPTY_OBJ, nullptr );
        for( const auto& item : resultSet ) {
            LOGGER_TRACE( cc::j( item ) );
        }
        EXPECT_EQ( resultSet.size(), expectedInserts );
    }
    catch( const std::exception& error ) {
        FAIL() << "Exception thrown: " << error.what();
    }
}

TEST_F( HouseModesOptionsMigrationTest, migrationV1_35_plugin_storages ) /* 27 */ {
    IDbClient& client = getDBClient();

    const std::string pluginStorage1 = "db.plugins.zwave.storage";
    const std::string pluginStorage2 = "db.plugins.blabla.storage";

    const std::string nonPluginStorage1 = "db.pluginsblabla.storage";
    const std::string nonPluginStorage2 = "db.plugins.blablastorage";

    const auto recordToFix = JSON( "_id"
                                   << "record_to_fix"
                                   << "value"
                                   << JSON( "a"
                                            << "A"
                                            << "\\"
                                            << "\\B"
                                            << "\\c"
                                            << "C\\"
                                            << "d\\"
                                            << "D"
                                            << "e"
                                            << JSON( "\\f"
                                                     << "F"
                                                     << "/"
                                                     << "G" ) ) );

    const auto recordToFix2 = JSON( "_id"
                                    << "\\record_to_fix2"
                                    << "value"
                                    << JSON_ARRAY( 1 << JSON( "x"
                                                              << "y" )
                                                     << JSON( "a" << JSON_ARRAY( JSON( "\\\\" << 1 ) ) ) ) );

    const auto recordWithoutKeysToFix = JSON( "_id"
                                              << "record_without_keys_to_fix"
                                              << "value"
                                              << JSON( "@"
                                                       << "a"
                                                       << "#"
                                                       << JSON_ARRAY( "$"
                                                                      << "\\" << 1 << true ) ) );

    const auto recordWithoutKeysToFix2 = JSON( "_id"
                                               << "\\record_without_keys_to_fix2"
                                               << "value"
                                               << "pam-param" );

    const auto fixedRecord = JSON( "_id"
                                   << "record_to_fix"
                                   << "value"
                                   << JSON( "a"
                                            << "A"
                                            << "\\\\"
                                            << "\\B"
                                            << "\\\\c"
                                            << "C\\"
                                            << "d\\"
                                            << "D"
                                            << "e"
                                            << JSON( "\\\\f"
                                                     << "F"
                                                     << "/"
                                                     << "G" ) ) );

    const auto fixedRecord2 = JSON( "_id"
                                    << "\\record_to_fix2"
                                    << "value"
                                    << JSON_ARRAY( 1 << JSON( "x"
                                                              << "y" )
                                                     << JSON( "a" << JSON_ARRAY( JSON( "\\\\\\" << 1 ) ) ) ) );

    client.insert( pluginStorage1, recordToFix );
    client.insert( pluginStorage2, recordToFix );
    client.insert( nonPluginStorage1, recordToFix );
    client.insert( nonPluginStorage2, recordToFix );

    client.insert( pluginStorage1, recordToFix2 );
    client.insert( pluginStorage2, recordToFix2 );
    client.insert( nonPluginStorage1, recordToFix2 );
    client.insert( nonPluginStorage2, recordToFix2 );

    client.insert( pluginStorage1, recordWithoutKeysToFix );
    client.insert( pluginStorage2, recordWithoutKeysToFix );
    client.insert( nonPluginStorage1, recordWithoutKeysToFix );
    client.insert( nonPluginStorage2, recordWithoutKeysToFix );

    client.insert( pluginStorage1, recordWithoutKeysToFix2 );
    client.insert( pluginStorage2, recordWithoutKeysToFix2 );
    client.insert( nonPluginStorage1, recordWithoutKeysToFix2 );
    client.insert( nonPluginStorage2, recordWithoutKeysToFix2 );

    MigrationTool::v1_35_plugin_storages::PluginStoragesMigration migration( client );
    EXPECT_TRUE( migration.migrate() );

    auto findOne = [&client]( const std::string& dbName, const std::string& recordId ) {
        return client.findOne( dbName, JSON( "_id" << recordId ), nullptr );
    };

    EXPECT_EQ( findOne( pluginStorage1, "record_to_fix" ), fixedRecord );
    EXPECT_EQ( findOne( pluginStorage2, "record_to_fix" ), fixedRecord );
    EXPECT_EQ( findOne( nonPluginStorage1, "record_to_fix" ), recordToFix );
    EXPECT_EQ( findOne( nonPluginStorage2, "record_to_fix" ), recordToFix );

    EXPECT_EQ( findOne( pluginStorage1, "\\record_to_fix2" ), fixedRecord2 );
    EXPECT_EQ( findOne( pluginStorage2, "\\record_to_fix2" ), fixedRecord2 );
    EXPECT_EQ( findOne( nonPluginStorage1, "\\record_to_fix2" ), recordToFix2 );
    EXPECT_EQ( findOne( nonPluginStorage2, "\\record_to_fix2" ), recordToFix2 );

    EXPECT_EQ( findOne( pluginStorage1, "record_without_keys_to_fix" ), recordWithoutKeysToFix );
    EXPECT_EQ( findOne( pluginStorage2, "record_without_keys_to_fix" ), recordWithoutKeysToFix );
    EXPECT_EQ( findOne( nonPluginStorage1, "record_without_keys_to_fix" ), recordWithoutKeysToFix );
    EXPECT_EQ( findOne( nonPluginStorage2, "record_without_keys_to_fix" ), recordWithoutKeysToFix );

    EXPECT_EQ( findOne( pluginStorage1, "\\record_without_keys_to_fix2" ), recordWithoutKeysToFix2 );
    EXPECT_EQ( findOne( pluginStorage2, "\\record_without_keys_to_fix2" ), recordWithoutKeysToFix2 );
    EXPECT_EQ( findOne( nonPluginStorage1, "\\record_without_keys_to_fix2" ), recordWithoutKeysToFix2 );
    EXPECT_EQ( findOne( nonPluginStorage2, "\\record_without_keys_to_fix2" ), recordWithoutKeysToFix2 );
}

TEST_F( MigrationTest, migrationV1_36 ) {
    const char ScenesDBName[] = "db.firmware.scenes_manager.rules";

    IDbClient& db = getDBClient();

    db.insert( ScenesDBName, JsonObject::parse( R"(
    {
        "_id": "1",
        "when": [
            {
                "_id": "1"
            },
            {
            },
            {
            }
        ],
        "then": [
            {
                "_id": "2"
            },
            {
            },
            {
            }
        ]
    } )" ) );

    db.insert( ScenesDBName, JsonObject::parse( R"(
    {
        "_id": "2",
        "when": [
            {
                "_id": "1"
            },
            {
            },
            {
            }
        ],
        "then": [
            {
                "_id": "2"
            },
            {
            },
            {
            }
        ]
    })" ) );

    MigrationTool::v1_36::Migration migrationTool( db );

    EXPECT_TRUE( migrationTool.migrate() );

    auto scenes = db.findAll( ScenesDBName, JSON_EMPTY_OBJ, nullptr );
    EXPECT_EQ( scenes.size(), 2 );

    for( const auto& scene : scenes ) {
        const auto& whenBlocks = scene[ "when" ];
        EXPECT_EQ( whenBlocks.size(), 3 );
        int i = 0;
        for( const auto& block : whenBlocks ) {
            if( i++ == 0 ) {
                EXPECT_EQ( json_get_str( block, "_id" ), "1" );
            }
            auto it = block.find( "_id" );
            EXPECT_TRUE( it != block.end() && it.value().is_string() );
        }
        const auto& thenBlocks = scene[ "then" ];
        EXPECT_EQ( thenBlocks.size(), 3 );
        i = 0;
        for( const auto& block : thenBlocks ) {
            if( i++ == 0 ) {
                EXPECT_EQ( json_get_str( block, "_id" ), "2" );
            }
            auto it = block.find( "_id" );
            EXPECT_TRUE( it != block.end() && it.value().is_string() );
        }
    }
}

TEST_F( MigrationTest, migrationV1_38NetworkStatisticSettings ) /* 29 */ {
    auto& client = getDBClient();

    EXPECT_EQ( client.findOne( "db.firmware.networkd.statistic.settings",
                               JSON( "_id"
                                     << "settings" ),
                               nullptr ),
               JSON_EMPTY_OBJ );

    MigrationTool::v1_38_network_statistic::Migration migrationTool( client );
    ASSERT_TRUE( migrationTool.migrate() );

    EXPECT_EQ( client.findOne( "db.firmware.networkd.statistic.settings",
                               JSON( "_id"
                                     << "settings" ),
                               nullptr ),
               JSON( "_id"
                     << "settings"
                     << "storageSize" << 30 ) );
}

TEST_F( MigrationTest, migrationV1_40bypassDevices ) /* 30 */ {
    namespace v1_40BypassDevices = MigrationTool::v1_40_bypass_devices;

    auto& dbClient = getDBClient();
    auto houseModesList = { std::cref( Resources::houseModeHome ),
                            std::cref( Resources::houseModeAway ),
                            std::cref( Resources::houseModeNight ),
                            std::cref( Resources::houseModeVacation ) };
    for( const Json& houseMode : houseModesList ) {
        ASSERT_NO_THROW( dbClient.insert( v1_40BypassDevices::DbNames::HaHouseModesDBName, houseMode ) );
    }

    v1_40BypassDevices::BypassDevicesMigration migration( dbClient );
    ASSERT_TRUE( migration.migrate() );

    std::vector< Json > expectedHouseModes;
    expectedHouseModes.reserve( 4 );
    for( const Json& houseMode : houseModesList ) {
        Json updated = houseMode;
        updated[ v1_40BypassDevices::Fields::bypassDevices ] = JSON_EMPTY_ARR;
        expectedHouseModes.push_back( std::move( updated ) );
    }

    std::sort( expectedHouseModes.begin(), expectedHouseModes.end(), []( const Json& left, const Json& right ) {
        const std::string leftId = json_get_str( left, v1_40BypassDevices::Fields::id );
        const std::string rightId = json_get_str( right, v1_40BypassDevices::Fields::id );
        return leftId < rightId;
    } );

    const std::vector< Json > dbRecords =
        dbClient.findAll( v1_40BypassDevices::DbNames::HaHouseModesDBName, Json(), nullptr );
    EXPECT_EQ( dbRecords, expectedHouseModes );
}

TEST_F( MigrationTest, migrationV1_41dateEventTimeParameterFix_simpleCase ) /* 31.1 */ {
    using namespace MigrationTool::v1_41_date_event_time_array_fix;

    auto& dbClient = getDBClient();
    ASSERT_NO_THROW( dbClient.insert( ScenesDb, v1_41_date_event_time_array_fix::simpleSceneWithErrorBlock ) );

    DateEventTimeFixMigration migration( dbClient );
    ASSERT_TRUE( migration.migrate() );

    const std::vector< Json > expected{ v1_41_date_event_time_array_fix::fixedSimpleScene };
    const std::vector< Json > dbRecords = dbClient.findAll( ScenesDb, Json(), nullptr );
    EXPECT_EQ( dbRecords, expected );
}

TEST_F( MigrationTest, migrationV1_41dateEventTimeParameterFix_complicatedCase ) /* 31.2 */ {
    using namespace MigrationTool::v1_41_date_event_time_array_fix;

    auto& dbClient = getDBClient();
    ASSERT_NO_THROW( dbClient.insert( ScenesDb, v1_41_date_event_time_array_fix::complicatedSceneWithErrorBlocks ) );

    DateEventTimeFixMigration migration( dbClient );
    ASSERT_TRUE( migration.migrate() );

    const std::vector< Json > expected{ v1_41_date_event_time_array_fix::fixedComplicatedScene };
    const std::vector< Json > dbRecords = dbClient.findAll( ScenesDb, Json(), nullptr );
    EXPECT_EQ( dbRecords, expected );
}

TEST_F( MigrationTest, migrationV1_42acquiredPlugins ) /* 32 */ {
    namespace v1_42AcquiredPlugins = MigrationTool::v1_42_acquired_plugins;

    auto& dbClient = getDBClient();

    std::vector< JsonObject > pluginsInfo = {
        JSON( "_id"
              << "62fb432800000045ade6a627"
              << "acquired" << false << "code"
              << "return 1"
              << "name"
              << "script1" ),
        JSON( "_id"
              << "62fb432800000045ade62345"
              << "acquired" << true << "code"
              << "return 2"
              << "name"
              << "script2" ),
        JSON( "_id"
              << "62fb432800000045ade22245"
              << "acquired" << false << "code"
              << "return 3"
              << "name"
              << "script3" ),
    };

    for( const Json& plugin : pluginsInfo ) {
        ASSERT_NO_THROW( dbClient.insert( v1_42AcquiredPlugins::DbNames::AnonymousPlugins, plugin ) );
    }

    v1_42AcquiredPlugins::AcquiredPluginsMigration migration( dbClient );
    ASSERT_TRUE( migration.migrate() );

    const std::vector< Json > dbRecords =
        dbClient.findAll( v1_42AcquiredPlugins::DbNames::AnonymousPlugins, Json(), nullptr );

    for( auto record : dbRecords ) {
        EXPECT_TRUE( record.find( "acquired" ) != record.end() );
        EXPECT_TRUE( record.find( "acquired" )->is_number_integer() );
        EXPECT_TRUE( record.find( "references" )->is_object() );
        JsonObject references;
        json_get_obj( record, "references", references );
        EXPECT_TRUE( references.find( "scenes" )->is_object() );
        EXPECT_EQ( json_get_int( record, "acquired" ), 0 );
    }
}

TEST_F( MigrationTest, migrationV1_43scriptTimeout ) /* 33 */ {
    namespace v1_43_script_timeout = MigrationTool::v1_43_script_timeout;

    auto& dbClient = getDBClient();
    v1_43_script_timeout::ScriptTimeoutMigration migration( dbClient );
    ASSERT_TRUE( migration.migrate() );

    const char LuaDefaultTimeoutKey[] = "lua.default_timeout";
    const char ValueKey[] = "value";
    const char ValueTypeKey[] = "valueType";

    int defaultTimeout = migration.getConfigTimeoutValue( ConfigKeys::LuadScriptTimeout );
    int minTimeout = migration.getConfigTimeoutValue( ConfigKeys::LuadScriptMinTimeout );

    auto defaultTimeoutRecord = dbClient.findOne( v1_43_script_timeout::DbNames::CoreSettingsDBName,
                                                  JSON( v1_43_script_timeout::Fields::id << LuaDefaultTimeoutKey ),
                                                  nullptr );

    EXPECT_TRUE( !defaultTimeoutRecord.empty() );
    EXPECT_EQ( json_get_int( defaultTimeoutRecord, ValueKey ), defaultTimeout );
    EXPECT_EQ( json_get_str( defaultTimeoutRecord, ValueTypeKey ), "int" );

    const char LuaMinTimeoutKey[] = "lua.min_timeout";
    auto minTimeoutRecord = dbClient.findOne( v1_43_script_timeout::DbNames::CoreSettingsDBName,
                                              JSON( v1_43_script_timeout::Fields::id << LuaMinTimeoutKey ),
                                              nullptr );

    EXPECT_TRUE( !minTimeoutRecord.empty() );
    EXPECT_EQ( json_get_int( minTimeoutRecord, ValueKey ), minTimeout );
    EXPECT_EQ( json_get_str( minTimeoutRecord, ValueTypeKey ), "int" );
}

TEST_F( MigrationTest, migrationV1_44deviceFollowParent ) /* 34 */ {
    namespace v1_44_device_parent_room = MigrationTool::v1_44_device_parent_room;

    auto& dbClient = getDBClient();

    std::vector< JsonObject > expected = deviceList;
    auto DevicesDBName = "db.firmware.cored.devices";

    auto device = deviceList[ 0 ];
    device[ "parentDeviceId" ] = "parent_id";
    dbClient.insert( DevicesDBName, device );           // create child
    dbClient.insert( DevicesDBName, deviceList[ 1 ] );  // parent

    v1_44_device_parent_room::DeviceParentRoomMigration migration( dbClient );
    ASSERT_TRUE( migration.migrate() );

    auto devices = dbClient.findAll( v1_44_device_parent_room::DbNames::DevicesDBName, JSON_EMPTY_OBJ, nullptr );
    ASSERT_EQ( expected.size(), devices.size() );
    for( auto device : devices ) {
        bool res;
        std::string parentId;
        if( json_get( device, "parentDeviceId", parentId ) && !parentId.empty() ) {
            ASSERT_TRUE( json_get( device, "parentRoom", res ) );
            EXPECT_EQ( res, true );
        }
        else {
            ASSERT_FALSE( json_get( device, "parentRoom", res ) );
        }
    }
}

TEST_F( MigrationTest, migrationV1_45_houseModeArmed ) /* 35 */ {
    namespace v1_40BypassDevices = MigrationTool::v1_40_bypass_devices;
    namespace v1_45 = MigrationTool::v1_45_house_mode_armed;

    auto& dbClient = getDBClient();
    auto houseModesList = { std::cref( Resources::houseModeHome ),
                            std::cref( Resources::houseModeAway ),
                            std::cref( Resources::houseModeNight ),
                            std::cref( Resources::houseModeVacation ) };
    for( const Json& houseMode : houseModesList ) {
        ASSERT_NO_THROW( dbClient.insert( v1_40BypassDevices::DbNames::HaHouseModesDBName, houseMode ) );
    }

    v1_45::HouseModeArmedMigration migration( dbClient );
    ASSERT_TRUE( migration.migrate() );

    const std::vector< Json > dbRecords = dbClient.findAll( v1_45::DbNames::HaHouseModesDBName, Json(), nullptr );

    for( auto& mode : dbRecords ) {
        auto id = json_get_str( mode, v1_45::Fields::id );
        const bool isArmed = id != "1";
        Json savedArmed;
        EXPECT_TRUE( json_get( mode, v1_45::Fields::armed, savedArmed ) );
        EXPECT_TRUE( savedArmed.is_boolean() );
        EXPECT_EQ( savedArmed.get< bool >(), isArmed );
    }
}

TEST_F( MigrationTest, migrationV1_48_rename_group_db ) /* 37 */ {
    namespace v1_48GroupDBRename = MigrationTool::v1_48;

    auto& dbClient = getDBClient();
    v1_48GroupDBRename::Migration migration( dbClient );

    auto oldDeviceGroupName = v1_48GroupDBRename::DbNames::HaScenesDeviceGroupsDBName;
    auto oldItemsGroupName = v1_48GroupDBRename::DbNames::HaScenesItemGroupsDBName;

    const auto DeviceNamePrefix = "deviceGroup";
    const auto ItemNamePrefix = "itemGroup";

    for( int i = 0; i < 10; i++ ) {
        std::string name = DeviceNamePrefix;
        name += std::to_string( i );
        ASSERT_NO_THROW(
            dbClient.insert( oldDeviceGroupName, JSON( "_id" << std::to_string( i ) << "name" << name ) ) );
    }

    for( int i = 0; i < 10; i++ ) {
        std::string name = ItemNamePrefix;
        name += std::to_string( i );
        ASSERT_NO_THROW( dbClient.insert( oldItemsGroupName, JSON( "_id" << std::to_string( i ) << "name" << name ) ) );
    }

    ASSERT_TRUE( migration.migrate() );

    auto newDeviceGroupName = v1_48GroupDBRename::DbNames::HaDeviceGroupsDBName;
    auto newItemsGroupName = v1_48GroupDBRename::DbNames::HaItemGroupsDBName;

    auto checkTable = []( std::vector< Json >& dbRecords, const std::string& namePrefix ) {
        for( auto& record : dbRecords ) {
            auto id = json_get_str( record, "_id" );
            std::string name = json_get_str( record, "name" );
            std::string expectedName = namePrefix;
            expectedName += id;
            EXPECT_EQ( name, expectedName );
        }
    };

    auto dbRecords = dbClient.findAll( newDeviceGroupName, Json(), nullptr );
    checkTable( dbRecords, DeviceNamePrefix );
    dbRecords = dbClient.findAll( newItemsGroupName, Json(), nullptr );
    checkTable( dbRecords, ItemNamePrefix );
}

TEST_F( MigrationTest, migrationV1_47_enable_protect_feature_on_e450 ) /* 36 */ {
    const char* const FeaturesTable = "db.firmware.cored.features";
    IDbClient& db = getDBClient();

    auto protectFeature = R"({
        "_id": "protect",
        "editable": true,
        "status": false,
        "version": "2.0"
    })"_json;

    auto miscFeature = R"({
        "_id": "misc",
        "editable": true,
        "status": false,
        "version": "1.0"
    })"_json;

    db.insert( FeaturesTable, protectFeature );
    db.insert( FeaturesTable, miscFeature );

    MigrationTool::v1_47_protect_feature_on_e450::Migration migrationTool( db );

    EXPECT_TRUE( migrationTool.migrate() );

    EXPECT_EQ( db.findAll( FeaturesTable, JSON_EMPTY_OBJ, nullptr ).size(), 2 );

    {
        auto expected = R"({
            "_id": "protect",
            "editable": true,
            "status": true,
            "version": "2.0"
        })"_json;

        auto actual = db.findOne( FeaturesTable,
                                  JSON( "_id"
                                        << "protect" ),
                                  nullptr );
        EXPECT_EQ( actual, expected );
    }

    {
        auto& expected = miscFeature;
        auto actual = db.findOne( FeaturesTable, JSON( Fields::id << "misc" ), nullptr );
        EXPECT_EQ( actual, expected );
    }
}

TEST_F( MigrationTest, migrationV1_49_house_mode_entry_delay ) /* 38 */ {
    const char* coredSettingsDb = "db.firmware.cored.settings";

    IDbClient& client = getDBClient();

    Json houseModesSettings = JSON( "_id"
                                    << "houseMode"
                                    << "alarmDelay" << 0 << "houseMode"
                                    << "1"
                                    << "switchToDelay" << 30 );
    Json Settings = JSON( "_id"
                          << "houseMode"
                          << "alarmDelay" << 0 << "houseMode"
                          << "1"
                          << "switchToDelay" << 30 << "entryDelay"
                          << JSON( "normal" << 30 << "short" << 30 << "extended" << 30 << "instant" << 0 ) );
    Json configSettings = JSON( "settings" << Settings );
    // create cored houseModes settings
    ASSERT_NO_THROW( client.insert( coredSettingsDb, houseModesSettings ) );

    class ConfigFactoryImpl : public UT::ConfigReaderFactory {
    public:
        using UT::ConfigReaderFactory::createConfigReader;
    };

    ConfigReader configReader = ConfigFactoryImpl::createConfigReader( [&]( const std::string& source ) {
        LOGGER_TRACE_RAW( "ConfigReader fake factory: " + cc::j( source ) + ", value: " + cc::j( configSettings ) );
        return configSettings.dump();
    } );

    MigrationTool::v1_49_house_mode_entry_delay::Migration migrationTool( client, &configReader );

    EXPECT_TRUE( migrationTool.migrate() );

    auto dbRecords = client.findAll( coredSettingsDb, JSON_EMPTY_OBJ, nullptr );
    EXPECT_FALSE( dbRecords.empty() );
    auto expected = R"({
            "normal": 30,
             "short": 30,
             "extended": 30,
             "instant": 0
        })"_json;
    for( auto& record : dbRecords ) {
        LOGGER_TRACE_RAW( cc::j( record ) );
        EXPECT_FALSE( record[ "entryDelay" ].empty() );
        auto actual = record[ "entryDelay" ];
        EXPECT_EQ( actual, expected );
    }
}

TEST_F( MigrationTest, migrationV1_50_device_group_new_fields ) /* 39 */ {
    const char* HaDeviceGroupsDBName = "db.firmware.cored.device_groups";

    IDbClient& client = getDBClient();

    Json deviceGroup = JSON( "_id"
                             << "123"
                             << "name"
                             << "testGroup"
                             << "categories"
                             << JSON_ARRAY( JSON( "category"
                                                  << "security_sensor"
                                                  << "subcategories" << JSON_ARRAY( "co2" ) )
                                            << JSON( "category"
                                                     << "scene_controller"
                                                     << "subcategories" << JSON_ARRAY( "panic" ) ) ) );

    ASSERT_NO_THROW( client.insert( HaDeviceGroupsDBName, deviceGroup ) );

    MigrationTool::v1_50_device_group_new_fields::Migration migrationTool( client );

    EXPECT_TRUE( migrationTool.migrate() );

    auto dbRecords = client.findAll( HaDeviceGroupsDBName, JSON_EMPTY_OBJ, nullptr );
    EXPECT_FALSE( dbRecords.empty() );

    for( auto& record : dbRecords ) {
        LOGGER_TRACE_RAW( cc::j( record ) );
        EXPECT_TRUE( json_contains( record, "entryDelay" ) );
        EXPECT_TRUE( json_contains( record, "followEntry" ) );
        EXPECT_TRUE( json_contains( record, "role" ) );
        EXPECT_TRUE( json_contains( record, "packageId" ) );

        EXPECT_EQ( json_get_str( record, "entryDelay" ), "none" );
        EXPECT_EQ( json_get_bool( record, "followEntry" ), false );
        EXPECT_EQ( json_get_str( record, "role" ), "user" );
        EXPECT_EQ( json_get_str( record, "packageId" ), "" );
    }
}

TEST_F( MigrationTest, migrationV1_51_vidoo_camera_devices ) /* 40 */ {
    const char* const DevicesTable = "db.firmware.cored.devices";
    IDbClient& db = getDBClient();

    const std::string vidooSuiteDeviceId = "000000000000000000000001";
    const std::string vidooCameraDeviceId = "000000000000000000000002";
    const std::string vidooSensorDeviceId = "000000000000000000000003";

    auto vidooSuiteDevice = R"({
        "_id": "000000000000000000000001",
        "batteryPowered": false,
        "category": "camera_suite",
        "deviceTypeId": "vidoo_device",
        "gatewayId": "648aedf500000000b07b7d94",
        "houseModesOptions": null,
        "info": {},
        "name": "Vidoo Device",
        "parentDeviceId": "",
        "persistent": false,
        "reachable": true,
        "ready": true,
        "roomId": "",
        "security": "no",
        "serviceNotification": false,
        "status": "idle",
        "subcategory": "vidoo",
        "type": "vidoo_device"
    })"_json;

    auto vidooCameraDevice = R"({
        "_id": "000000000000000000000002",
        "batteryPowered": false,
        "category": "camera",
        "deviceTypeId": "vidoo_generic_cam",
        "gatewayId": "648aedf500000000b07b7d94",
        "houseModesOptions": null,
        "info": {},
        "name": "cameraName1",
        "parentDeviceId": "000000000000000000000001",
        "parentRoom": true,
        "persistent": false,
        "reachable": true,
        "ready": true,
        "roomId": "",
        "security": "no",
        "serviceNotification": false,
        "status": "idle",
        "subcategory": "doorbell_cam",
        "type": "vidoo_generic_cam"
    })"_json;

    auto vidooSensorDevice = R"({
        "_id": "000000000000000000000003",
        "armed": true,
        "batteryPowered": false,
        "category": "security_sensor",
        "deviceTypeId": "motion",
        "gatewayId": "648aedf500000000b07b7d94",
        "houseModesOptions": null,
        "info": {},
        "name": "sensorof[cameraName1]",
        "parentDeviceId": "000000000000000000000002",
        "parentRoom": true,
        "persistent": false,
        "reachable": true,
        "ready": true,
        "roomId": "",
        "security": "no",
        "serviceNotification": false,
        "status": "idle",
        "subcategory": "motion",
        "type": "vidoo_motion_sensor"
    })"_json;

    db.insert( DevicesTable, vidooSuiteDevice );
    db.insert( DevicesTable, vidooCameraDevice );
    db.insert( DevicesTable, vidooSensorDevice );

    MigrationTool::v1_51_vidoo_camera_devices::Migration migrationTool( db );

    EXPECT_TRUE( migrationTool.migrate() );

    EXPECT_EQ( db.findAll( DevicesTable, JSON_EMPTY_OBJ, nullptr ).size(), 3 );

    {  // Parent device should be removed from the camera device
        auto device = db.findOne( DevicesTable, JSON( "_id" << vidooCameraDeviceId ), nullptr );
        EXPECT_TRUE( !device.empty() );
        EXPECT_TRUE( json_get_str( device, "parentDeviceId" ).empty() );
    }

    {  // Sensor device should be unchanged
        auto device = db.findOne( DevicesTable, JSON( "_id" << vidooSensorDeviceId ), nullptr );
        EXPECT_EQ( json_get_str( device, "parentDeviceId" ), vidooCameraDeviceId );
    }
}

TEST_F( MigrationTest, migrationV1_52_house_mode_alarm_delay_update ) /* 41 */ {
    {
        const char* coredSettingsDb = "db.firmware.cored.settings";

        IDbClient& client = getDBClient();
        auto expectedValue = 30;
        Json houseModesSettings = JSON( "_id"
                                        << "houseMode"
                                        << "alarmDelay" << 0 << "houseMode"
                                        << "1"
                                        << "switchToDelay" << 30 << "entryDelay"
                                        << JSON( "normal" << expectedValue << "short" << 30 << "extended" << 30
                                                          << "instant" << 0 ) );

        ASSERT_NO_THROW( client.insert( coredSettingsDb, houseModesSettings ) );

        MigrationTool::v1_52_house_mode_alarm_delay_update::Migration migrationTool( client );

        EXPECT_TRUE( migrationTool.migrate() );

        auto dbRecords = client.findAll( coredSettingsDb, JSON_EMPTY_OBJ, nullptr );
        EXPECT_FALSE( dbRecords.empty() );

        for( auto& record : dbRecords ) {
            LOGGER_TRACE_RAW( cc::j( record ) );
            auto entryDelay = record[ "entryDelay" ];
            EXPECT_EQ( record[ "alarmDelay" ], entryDelay[ "normal" ] );
            EXPECT_EQ( record[ "alarmDelay" ], expectedValue );
            EXPECT_EQ( entryDelay[ "normal" ], expectedValue );
        }
    }
    {
        const char* coredSettingsDb = "db.firmware.cored.settings";

        IDbClient& client = getDBClient();
        auto expectedValue = 20;
        Json houseModesSettings = JSON( "_id"
                                        << "houseMode"
                                        << "alarmDelay" << 20 << "houseMode"
                                        << "1"
                                        << "switchToDelay" << 30 << "entryDelay"
                                        << JSON( "normal" << 30 << "short" << 30 << "extended" << 30 << "instant"
                                                          << 0 ) );

        ASSERT_NO_THROW( client.insert( coredSettingsDb, houseModesSettings ) );

        MigrationTool::v1_52_house_mode_alarm_delay_update::Migration migrationTool( client );

        EXPECT_TRUE( migrationTool.migrate() );

        auto dbRecords = client.findAll( coredSettingsDb, JSON_EMPTY_OBJ, nullptr );
        EXPECT_FALSE( dbRecords.empty() );

        for( auto& record : dbRecords ) {
            LOGGER_TRACE_RAW( cc::j( record ) );
            auto entryDelay = record[ "entryDelay" ];
            EXPECT_EQ( record[ "alarmDelay" ], entryDelay[ "normal" ] );
            EXPECT_EQ( record[ "alarmDelay" ], expectedValue );
            EXPECT_EQ( entryDelay[ "normal" ], expectedValue );
        }
    }
}

TEST_F( MigrationTest, v1_54HouseModeGlassBreakageDeviceArmedStatusChanged ) {
    const char* devicesDb = "db.firmware.cored.devices";
    const char* houseModesDb = "db.firmware.cored.houseModes";
    const char* coredSettingsDb = "db.firmware.cored.settings";

    Json houseModesSettings = JSON( "_id"
                                    << "houseMode"
                                    << "alarmDelay" << 20 << "houseMode"
                                    << "1"
                                    << "switchToDelay" << 30 );

    IDbClient& dbClient = getDBClient();
    ASSERT_NO_THROW( dbClient.insert( coredSettingsDb, houseModesSettings ) );

    auto device = JSON( "_id"
                        << "glass_breakage"
                        << "category"
                        << "security_sensor"
                        << "deviceTypeId"
                        << "sensor"
                        << "name"
                        << "Glass Breakage Sensor"
                        << "subcategory"
                        << "glass"
                        << "type"
                        << "sensor"
                        << "armed" << true );

    // Create glass sensor device
    dbClient.insert( devicesDb, device );
    auto glassSensorDevice = dbClient.findOne( devicesDb,
                                               JSON( "_id"
                                                     << "glass_breakage" ),
                                               nullptr );
    EXPECT_TRUE( json_get_bool( glassSensorDevice, "armed" ) );

    // create houseMode
    ASSERT_NO_THROW( dbClient.insert( houseModesDb, Resources::houseModeHome ) );

    MigrationTool::v1_54_house_mode_glass_breakage_sensor::Migration migrationTool( dbClient );

    EXPECT_TRUE( migrationTool.migrate() );

    glassSensorDevice = dbClient.findOne( devicesDb,
                                          JSON( "_id"
                                                << "glass_breakage" ),
                                          nullptr );
    EXPECT_FALSE( json_get_bool( glassSensorDevice, "armed" ) );
}

TEST_F( MigrationTest, v1_54HouseModeGlassBreakageDeviceArmedStatusNotChanged ) {
    const char* devicesDb = "db.firmware.cored.devices";
    const char* houseModesDb = "db.firmware.cored.houseModes";
    const char* coredSettingsDb = "db.firmware.cored.settings";

    Json houseModesSettings = JSON( "_id"
                                    << "houseMode"
                                    << "alarmDelay" << 20 << "houseMode"
                                    << "2"
                                    << "switchToDelay" << 30 );

    IDbClient& dbClient = getDBClient();
    ASSERT_NO_THROW( dbClient.insert( coredSettingsDb, houseModesSettings ) );

    auto device = JSON( "_id"
                        << "glass_breakage"
                        << "category"
                        << "security_sensor"
                        << "deviceTypeId"
                        << "sensor"
                        << "name"
                        << "Glass Breakage Sensor"
                        << "subcategory"
                        << "glass"
                        << "type"
                        << "sensor"
                        << "armed" << true );

    // Create glass sensor device
    dbClient.insert( devicesDb, device );
    auto glassSensorDevice = dbClient.findOne( devicesDb,
                                               JSON( "_id"
                                                     << "glass_breakage" ),
                                               nullptr );
    EXPECT_TRUE( json_get_bool( glassSensorDevice, "armed" ) );

    // create houseMode
    ASSERT_NO_THROW( dbClient.insert( houseModesDb, Resources::houseModeAway ) );

    MigrationTool::v1_54_house_mode_glass_breakage_sensor::Migration migrationTool( dbClient );

    EXPECT_TRUE( migrationTool.migrate() );

    glassSensorDevice = dbClient.findOne( devicesDb,
                                          JSON( "_id"
                                                << "glass_breakage" ),
                                          nullptr );
    EXPECT_TRUE( json_get_bool( glassSensorDevice, "armed" ) );
}

TEST_F( MigrationTest, v1_55ProtectConfig ) {
    const auto DevicesDBName = "db.firmware.cored.devices";
    const std::string ProtectConfig = "protectConfig";
    const std::string ProtectConfigDefault = "default";
    const std::string ProtectConfigOther = "other";
    const std::string Device1 = "device1";
    const std::string Device2 = "device2";
    const std::string Device3 = "device3";
    const std::string Device4 = "device4";
    const std::string Id = "_id";
    const std::string Devices = "devices";

    IDbClient& dbClient = getDBClient();

    dbClient.insert( DevicesDBName, JSON( Id << Device1 ) );
    dbClient.insert( DevicesDBName, JSON( Id << Device2 ) );
    dbClient.insert( DevicesDBName, JSON( Id << Device3 ) );
    dbClient.insert( DevicesDBName, JSON( Id << Device4 << ProtectConfig << ProtectConfigOther ) );

    MigrationTool::v1_55_protect_config::Migration migrationTool( dbClient );

    EXPECT_TRUE( migrationTool.migrate() );

    auto devs = dbClient.findAll( DevicesDBName, JSON_EMPTY_OBJ, nullptr );
    auto device = dbClient.findOne( DevicesDBName, JSON( Id << Device1 ), nullptr );
    std::string protectConfig = json_get_str( device, ProtectConfig );
    EXPECT_EQ( protectConfig, ProtectConfigDefault );

    device = dbClient.findOne( DevicesDBName, JSON( Id << Device2 ), nullptr );
    protectConfig = json_get_str( device, ProtectConfig );
    EXPECT_EQ( protectConfig, ProtectConfigDefault );

    device = dbClient.findOne( DevicesDBName, JSON( Id << Device3 ), nullptr );
    protectConfig = json_get_str( device, ProtectConfig );
    EXPECT_EQ( protectConfig, ProtectConfigDefault );

    device = dbClient.findOne( DevicesDBName, JSON( Id << Device4 ), nullptr );
    protectConfig = json_get_str( device, ProtectConfig );
    EXPECT_EQ( protectConfig, ProtectConfigOther );
}

TEST_F( MigrationTest, v1_56ZigbeeDevices_DoubleReachableCheckTimeout ) {
    auto zigbeeDbName = "db.addons.zigbee.devices";

    auto Id = "_id";
    auto isBatteryPowered = "is_battery_powered";
    auto manufacturerId = "manufacturer_id";
    auto reachableStateTimeout = "reachable_state_timeout";
    auto someKey = "some_key";
    auto someValue = "some_value";

    auto device1 = JSON( Id << 1 << isBatteryPowered << true << manufacturerId << "Centralites" << reachableStateTimeout
                            << 3600 << someKey << someValue );
    auto device2 = JSON( Id << 2 << isBatteryPowered << false << manufacturerId << "Samsung" << reachableStateTimeout
                            << 5300 << someKey << someValue );
    auto device3 = JSON( Id << 3 << reachableStateTimeout << 1000 );
    auto device4 = JSON( Id << 4 << isBatteryPowered << 15 << manufacturerId << true << reachableStateTimeout << "aaa"
                            << someKey << someValue );

    IDbClient& dbClient = getDBClient();
    dbClient.insert( zigbeeDbName, device1 );
    dbClient.insert( zigbeeDbName, device2 );
    dbClient.insert( zigbeeDbName, device3 );
    dbClient.insert( zigbeeDbName, device4 );

    MigrationTool::v1_56_zigbee_devices::Migration migrationTool( dbClient );

    EXPECT_TRUE( migrationTool.migrate() );

    device1[ reachableStateTimeout ] = 7200;
    device2[ reachableStateTimeout ] = 10600;
    device3[ reachableStateTimeout ] = 2000;
    device4[ reachableStateTimeout ] = 0;

    auto device1FromDb = dbClient.findOne( zigbeeDbName, JSON( Id << 1 ), nullptr );
    auto device2FromDb = dbClient.findOne( zigbeeDbName, JSON( Id << 2 ), nullptr );
    auto device3FromDb = dbClient.findOne( zigbeeDbName, JSON( Id << 3 ), nullptr );
    auto device4FromDb = dbClient.findOne( zigbeeDbName, JSON( Id << 4 ), nullptr );

    EXPECT_EQ( device1FromDb.dump(), device1.dump() );
    EXPECT_EQ( device2FromDb.dump(), device2.dump() );
    EXPECT_EQ( device3FromDb.dump(), device3.dump() );
    EXPECT_EQ( device4FromDb.dump(), device4.dump() );
}

TEST_F( MigrationTest, v1_56ZigbeeDevices_MarkCentraliteDevicesAsSleepy ) {
    auto zigbeeDbName = "db.addons.zigbee.devices";

    auto Id = "_id";
    auto isBatteryPowered = "is_battery_powered";
    auto manufacturerId = "manufacturer_id";
    auto reachableStateTimeout = "reachable_state_timeout";
    auto isSleepy = "is_sleepy";
    auto someKey = "some_key";
    auto someValue = "some_value";

    auto device1 = JSON( Id << 1 << isBatteryPowered << true << manufacturerId << "Centralite" << reachableStateTimeout
                            << 0 << someKey << someValue );
    auto device2 = JSON( Id << 2 << isBatteryPowered << true << manufacturerId << "CentraLite" << reachableStateTimeout
                            << 0 << someKey << someValue );
    auto device3 = JSON( Id << 3 << isBatteryPowered << false << manufacturerId << "Centralite" << reachableStateTimeout
                            << 0 << someKey << someValue );
    auto device4 = JSON( Id << 4 << isBatteryPowered << true << manufacturerId << "Centralites" << reachableStateTimeout
                            << 0 << someKey << someValue );

    auto device5 = JSON( Id << 5 << reachableStateTimeout << 0 );
    auto device6 = JSON( Id << 6 << isBatteryPowered << 15 << manufacturerId << true << reachableStateTimeout << 0
                            << someKey << someValue );

    IDbClient& dbClient = getDBClient();
    dbClient.insert( zigbeeDbName, device1 );
    dbClient.insert( zigbeeDbName, device2 );
    dbClient.insert( zigbeeDbName, device3 );
    dbClient.insert( zigbeeDbName, device4 );
    dbClient.insert( zigbeeDbName, device5 );
    dbClient.insert( zigbeeDbName, device6 );

    MigrationTool::v1_56_zigbee_devices::Migration migrationTool( dbClient );

    EXPECT_TRUE( migrationTool.migrate() );

    device1[ isSleepy ] = true;
    device2[ isSleepy ] = true;

    auto device1FromDb = dbClient.findOne( zigbeeDbName, JSON( Id << 1 ), nullptr );
    auto device2FromDb = dbClient.findOne( zigbeeDbName, JSON( Id << 2 ), nullptr );
    auto device3FromDb = dbClient.findOne( zigbeeDbName, JSON( Id << 3 ), nullptr );
    auto device4FromDb = dbClient.findOne( zigbeeDbName, JSON( Id << 4 ), nullptr );
    auto device5FromDb = dbClient.findOne( zigbeeDbName, JSON( Id << 5 ), nullptr );
    auto device6FromDb = dbClient.findOne( zigbeeDbName, JSON( Id << 6 ), nullptr );

    EXPECT_EQ( device1FromDb.dump(), device1.dump() );
    EXPECT_EQ( device2FromDb.dump(), device2.dump() );
    EXPECT_EQ( device3FromDb.dump(), device3.dump() );
    EXPECT_EQ( device4FromDb.dump(), device4.dump() );
    EXPECT_EQ( device5FromDb.dump(), device5.dump() );
    EXPECT_EQ( device6FromDb.dump(), device6.dump() );
}

}  // namespace UT
}  // namespace HubPlatform
