#include "scenes_manager/tests/resources/scene_scale_resources.h"
#include "migration_tool/migration/v1_37_securityDevices/security_devices_migration.h"
#include "migration_tool/migration/v1_39_security_threat/security_threat_migration.h"

#include "migration_tool/tests/migration_test_fixture.h"
#include "test_utils/temporary_root_folder_wrapper.h"
#include "database_client/idbclient.h"
#include "database_client/dbqueryparser.h"
#include "system_utils/environment.h"
#include "system_utils/filesystem.h"
#include "log_utils/ezlo_log.h"
#include "json_utils/json_utils.h"

#include <gtest/gtest.h>

/*

TESTPLAN
DONE    1. Migrate data to 1.37: House modes security devices
DONE        1.1 Device with security_threat item, added to house modes properly (add to security devices)
DONE        1.2 Device with security_threat item, not matching the categories list (to be added)
DONE        1.3 Device with security_threat item (to be added)
DONE        1.4 Device without security_threat item, added to house modes by categories (to be deleted)

DONE    2. Migration 1.39: set device.armed=false eif there is no item named 'security_threat'.

*/

namespace HubPlatform {
namespace UT {

namespace v1_37 {
namespace Resources {

Json houseModeHome = R"({
    "_id": "1",
    "name": "Home",
    "armedCategories": [],
    "alarmCategories": [{ "category": "siren" }],
    "alarmsOffDevices": [],
    "disarmedDevices": [],
    "disarmedDefault": true
})"_json;

Json houseModeAway = R"({
    "_id": "2",
    "name": "Away",
    "armedCategories": [],
    "alarmCategories": [{ "category": "siren" }],
    "alarmsOffDevices": [],
    "disarmedDevices": [],
    "disarmedDefault": true
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

}  // namespace Resources
}  // namespace v1_37

class HouseModesSecurityDevices_V1_37_MigrationTest : public MigrationTestFixture {
public:
    void initHouseModes( const std::vector< std::string >& disarmedDevices, const char* armedSubcategory = "co" );
};

void HouseModesSecurityDevices_V1_37_MigrationTest::initHouseModes( const std::vector< std::string >& disarmedDevices,
                                                                    const char* armedSubcategory ) {
    using namespace v1_37;
    namespace Fields = HubPlatform::MigrationTool::v1_37_securityDevices::Fields;
    namespace Tables = HubPlatform::MigrationTool::v1_37_securityDevices::Tables;
    namespace Values = HubPlatform::MigrationTool::v1_37_securityDevices::Values;

    auto modes = {
        Resources::houseModeHome, Resources::houseModeAway, Resources::houseModeNight, Resources::houseModeVacation
    };
    for( auto mode : modes ) {
        Json& armedCategories = mode[ Fields::armedCategories ];
        armedCategories.push_back( JSON( Fields::category << Values::securitySensor << Fields::subcategories
                                                          << JSON_ARRAY_( armedSubcategory ) ) );
        if( !disarmedDevices.empty() ) {
            mode[ Fields::disarmedDevices ] = disarmedDevices;
        }

        ASSERT_NO_THROW( getDBClient().insert( Tables::HouseModesDb, mode ) );
    }
}

TEST_F( HouseModesSecurityDevices_V1_37_MigrationTest, properlyAddedDevice ) /* 1.1 */ {
    Log::setLocalLogLevel( Log::PriorityLevel::TRACE );
    namespace Fields = HubPlatform::MigrationTool::v1_37_securityDevices::Fields;
    namespace Tables = HubPlatform::MigrationTool::v1_37_securityDevices::Tables;
    namespace Values = HubPlatform::MigrationTool::v1_37_securityDevices::Values;

    using namespace v1_37;
    IDbClient& client = getDBClient();
    const char armedSubcategory[] = "co";
    const char* deviceName = test_info_->name();

    auto initDeviceItems = [this, &armedSubcategory]( const char* deviceName ) {
        auto& client = getDBClient();

        Json device = Resources::vistaCamMotionSensor;
        device[ Fields::id ] = deviceName;
        device[ Fields::type ] = Values::sensor;
        device[ Fields::category ] = Values::securitySensor;
        device[ Fields::subcategory ] = armedSubcategory;
        device[ "deviceTypeId" ] = "test_sensor";
        device[ "parentDeviceId" ] = "";
        device[ Fields::name ] = deviceName;
        device[ Fields::armed ] = true;

        client.insert( Tables::DevicesDb, device );

        Json item = { { Fields::id, "security_threat_id1" },
                      { Fields::name, Values::securityThreat },
                      { Fields::hasSetter, false },
                      { Fields::hasGetter, true },
                      { Fields::show, true },
                      { Fields::value, 0 },
                      { Fields::type, "int" },
                      { Fields::deviceId, deviceName },
                      { Fields::enumField, JSON_EMPTY_ARR } };
        client.insert( Tables::ItemsDb, item );
    };

    initHouseModes( {}, armedSubcategory );
    initDeviceItems( deviceName );

    const size_t expectedSecurityDevices = 1;
    const size_t expectedSecurityDevicesAdded = 1;
    const size_t expectedInsertedInModes = 0;

    MigrationTool::v1_37_securityDevices::SecurityDeviceMigration migrationTool( client );
    try {
        EXPECT_TRUE( migrationTool.migrate() );
        EXPECT_EQ( migrationTool.insertedInModes(), expectedInsertedInModes )
            << "Disarmed device ids inserted while migrating";

        EXPECT_EQ( migrationTool.securityDevicesAdded(), expectedSecurityDevicesAdded );
        auto securityDevices = migrationTool.fetchSecurityDevices();
        EXPECT_EQ( securityDevices.size(), expectedSecurityDevices ) << "Should NOT be empty";

        auto houseModes = migrationTool.fetchHouseModes();

        // check modification in every house mode
        for( const auto& hmIt : houseModes ) {
            const auto& modeId = hmIt.first;
            const Json& mode = hmIt.second;

            JsonArray disarmedDevices;
            EXPECT_TRUE( json_get_arr( mode, Fields::disarmedDevices, disarmedDevices ) );
            EXPECT_TRUE( disarmedDevices.empty() ) << "Should not be added to any house mode";
        }

        auto device = migrationTool.fetchDevice( deviceName );
        EXPECT_FALSE( device.empty() ) << "device should not be empty";
        LOGGER_TRACE( "DEVICE: " + cc::j( device ) );
        EXPECT_TRUE( device.count( Fields::armed ) ) << "should have field: armed";

        EXPECT_EQ( device[ Fields::armed ], true ) << "Should be armed in modeId=1, device.armed=true";
    }
    catch( const std::exception& error ) {
        FAIL() << "Exception thrown: " << error.what();
    }
}

TEST_F( HouseModesSecurityDevices_V1_37_MigrationTest, addDeviceWithSecurityThreatNotMatchingCategories ) /* 1.2 */ {
    Log::setLocalLogLevel( Log::PriorityLevel::TRACE );
    namespace Fields = HubPlatform::MigrationTool::v1_37_securityDevices::Fields;
    namespace Tables = HubPlatform::MigrationTool::v1_37_securityDevices::Tables;
    namespace Values = HubPlatform::MigrationTool::v1_37_securityDevices::Values;

    using namespace v1_37;
    IDbClient& client = getDBClient();
    const char armedSubcategory[] = "co";
    const char* deviceName = test_info_->name();

    auto initDeviceItems = [this]( const char* deviceName ) {
        auto& client = getDBClient();

        Json device = Resources::vistaCamMotionSensor;
        device[ Fields::id ] = deviceName;
        device[ Fields::type ] = Values::sensor;
        device[ Fields::category ] = Values::securitySensor;
        device[ Fields::subcategory ] = "not_in_armed_subcategories";  // ?
        device[ "deviceTypeId" ] = "test_sensor";
        device[ "parentDeviceId" ] = "";
        device[ Fields::name ] = deviceName;

        client.insert( Tables::DevicesDb, device );

        Json item = { { Fields::id, "security_threat_id1" },
                      { Fields::name, Values::securityThreat },
                      { Fields::hasSetter, false },
                      { Fields::hasGetter, true },
                      { Fields::show, true },
                      { Fields::value, 0 },
                      { Fields::type, "int" },
                      { Fields::deviceId, deviceName },
                      { Fields::enumField, JSON_EMPTY_ARR } };
        client.insert( Tables::ItemsDb, item );
    };

    initHouseModes( {}, armedSubcategory );
    initDeviceItems( deviceName );

    const size_t expectedSecurityDevices = 1;
    const size_t expectedSecurityDevicesAdded = 1;
    const size_t expectedInsertedInModes = 4;

    MigrationTool::v1_37_securityDevices::SecurityDeviceMigration migrationTool( client );
    try {
        EXPECT_TRUE( migrationTool.migrate() );
        EXPECT_EQ( migrationTool.insertedInModes(), expectedInsertedInModes )
            << "Disarmed device ids inserted while migrating";

        EXPECT_EQ( migrationTool.securityDevicesAdded(), expectedSecurityDevicesAdded );
        auto securityDevices = migrationTool.fetchSecurityDevices();
        EXPECT_EQ( securityDevices.size(), expectedSecurityDevices ) << "Should NOT be empty";

        auto houseModes = migrationTool.fetchHouseModes();

        // check modification in every house mode
        for( const auto& hmIt : houseModes ) {
            const auto& modeId = hmIt.first;
            const Json& mode = hmIt.second;

            JsonArray disarmedDevices;
            EXPECT_TRUE( json_get_arr( mode, Fields::disarmedDevices, disarmedDevices ) );
            EXPECT_TRUE( !disarmedDevices.empty() ) << "Should be added from every house mode";
        }

        auto device = migrationTool.fetchDevice( deviceName );
        EXPECT_FALSE( device.empty() ) << "device should ot be empty";
        LOGGER_TRACE( "DEVICE: " + cc::j( device ) );
        EXPECT_TRUE( device.count( Fields::armed ) ) << "should have field: armed";
        EXPECT_EQ( device[ Fields::armed ], false ) << "Should be disarmed in modeId=1, device.armed=false";
    }
    catch( const std::exception& error ) {
        FAIL() << "Exception thrown: " << error.what();
    }
}

TEST_F( HouseModesSecurityDevices_V1_37_MigrationTest, addDeviceWithSecurityThreatMatchingArmedCategories ) /* 1.3 */ {
    Log::setLocalLogLevel( Log::PriorityLevel::TRACE );
    namespace Fields = HubPlatform::MigrationTool::v1_37_securityDevices::Fields;
    namespace Tables = HubPlatform::MigrationTool::v1_37_securityDevices::Tables;
    namespace Values = HubPlatform::MigrationTool::v1_37_securityDevices::Values;

    using namespace v1_37;
    IDbClient& client = getDBClient();
    const char armedSubcategory[] = "co";
    const char* deviceName = test_info_->name();

    auto initDeviceItems = [this, &armedSubcategory]( const char* deviceName ) {
        auto& client = getDBClient();

        Json device = Resources::vistaCamMotionSensor;
        device[ Fields::id ] = deviceName;
        device[ Fields::type ] = Values::sensor;
        device[ Fields::category ] = Values::securitySensor;
        device[ Fields::subcategory ] = armedSubcategory;
        device[ "deviceTypeId" ] = "test_sensor";
        device[ "parentDeviceId" ] = "";
        device[ Fields::name ] = deviceName;
        device[ Fields::armed ] = false;

        client.insert( Tables::DevicesDb, device );

        Json item = { { Fields::id, "security_threat_id1" },
                      { Fields::name, Values::securityThreat },
                      { Fields::hasSetter, false },
                      { Fields::hasGetter, true },
                      { Fields::show, true },
                      { Fields::value, 0 },
                      { Fields::type, "int" },
                      { Fields::deviceId, deviceName },
                      { Fields::enumField, JSON_EMPTY_ARR } };
        client.insert( Tables::ItemsDb, item );
    };

    initHouseModes( {}, armedSubcategory );
    initDeviceItems( deviceName );

    const size_t expectedSecurityDevices = 1;
    const size_t expectedSecurityDevicesAdded = 1;
    const size_t expectedInsertedInModes = 0;

    MigrationTool::v1_37_securityDevices::SecurityDeviceMigration migrationTool( client );
    try {
        EXPECT_TRUE( migrationTool.migrate() );
        EXPECT_EQ( migrationTool.insertedInModes(), expectedInsertedInModes )
            << "Disarmed device ids inserted while migrating";

        EXPECT_EQ( migrationTool.securityDevicesAdded(), expectedSecurityDevicesAdded );
        auto securityDevices = migrationTool.fetchSecurityDevices();
        EXPECT_EQ( securityDevices.size(), expectedSecurityDevices ) << "Should NOT be empty";

        auto houseModes = migrationTool.fetchHouseModes();

        // check modification in every house mode
        for( const auto& hmIt : houseModes ) {
            const auto& modeId = hmIt.first;
            const Json& mode = hmIt.second;

            JsonArray disarmedDevices;
            EXPECT_TRUE( json_get_arr( mode, Fields::disarmedDevices, disarmedDevices ) );
            EXPECT_TRUE( disarmedDevices.empty() ) << "Should not be added to any house mode";
        }

        auto device = migrationTool.fetchDevice( deviceName );
        EXPECT_FALSE( device.empty() ) << "device should not be empty";
        LOGGER_TRACE( "DEVICE: " + cc::j( device ) );
        EXPECT_TRUE( device.count( Fields::armed ) ) << "should have field: armed";

        EXPECT_EQ( device[ Fields::armed ], false ) << "Should be armed in modeId=1, device.armed=true";
    }
    catch( const std::exception& error ) {
        FAIL() << "Exception thrown: " << error.what();
    }
}

TEST_F( HouseModesSecurityDevices_V1_37_MigrationTest, deleteArmedWOSecurityThreatAddedByCategories ) /* 1.4 */ {
    Log::setLocalLogLevel( Log::PriorityLevel::TRACE );
    namespace Fields = HubPlatform::MigrationTool::v1_37_securityDevices::Fields;
    namespace Tables = HubPlatform::MigrationTool::v1_37_securityDevices::Tables;
    namespace Values = HubPlatform::MigrationTool::v1_37_securityDevices::Values;

    using namespace v1_37;
    IDbClient& client = getDBClient();
    const char armedSubcategory[] = "co";

    const char* deviceName = test_info_->name();
    auto initDeviceItems = [this, &armedSubcategory]( const char* deviceName ) {
        auto& client = getDBClient();
        if( !deviceName || !*deviceName )
            throw std::runtime_error( "Empty device name" );

        Json device = Resources::vistaCamMotionSensor;
        device[ Fields::id ] = deviceName;
        device[ Fields::subcategory ] = armedSubcategory;
        device[ Fields::name ] = deviceName;
        device[ Fields::armed ] = true;
        client.insert( Tables::DevicesDb, device );
    };

    initHouseModes( { deviceName }, armedSubcategory );
    initDeviceItems( deviceName );

    const int expectedRemovedFromDisarmedDevices = 1;

    MigrationTool::v1_37_securityDevices::SecurityDeviceMigration migrationTool( client );
    try {
        EXPECT_TRUE( migrationTool.migrate() );
        EXPECT_EQ( migrationTool.removedFromDisarmedDevices(), expectedRemovedFromDisarmedDevices );

        auto securityDevices = migrationTool.fetchSecurityDevices();
        EXPECT_EQ( securityDevices.size(), 0 ) << "Should be empty";
        EXPECT_EQ( migrationTool.changedArmedTo( false ), 1 ) << "device.armed should be changed to false";

        auto houseModes = migrationTool.fetchHouseModes();

        // check modification in every house mode
        for( const auto& hmIt : houseModes ) {
            const auto& modeId = hmIt.first;
            const Json& mode = hmIt.second;

            JsonArray disarmedDevices;
            EXPECT_TRUE( json_get_arr( mode, Fields::disarmedDevices, disarmedDevices ) );
            EXPECT_TRUE( disarmedDevices.empty() ) << "Should be deleted from every house mode";
        }

        auto device = migrationTool.fetchDevice( deviceName );
        EXPECT_FALSE( device.empty() ) << "device should not be empty";
        LOGGER_TRACE( "DEVICE: " + cc::j( device ) );
        EXPECT_TRUE( device.count( Fields::armed ) ) << "should have field: armed";
        EXPECT_EQ( device[ Fields::armed ], false ) << "Should be disarmed in modeId=1, device.armed=false";
    }
    catch( const std::exception& error ) {
        FAIL() << "Exception thrown: " << error.what();
    }
}

class SecurityDevices_V1_39_MigrationTest : public MigrationTestFixture {};

TEST_F( SecurityDevices_V1_39_MigrationTest, deviceArmedToFalseIfNoSecurityThreat_T2 ) /* 2 */ {
    namespace Fields = HubPlatform::MigrationTool::v1_37_securityDevices::Fields;
    namespace Tables = HubPlatform::MigrationTool::v1_37_securityDevices::Tables;
    namespace Values = HubPlatform::MigrationTool::v1_37_securityDevices::Values;
    using namespace MigrationTool::v1_39_security_threat;
    IDbClient& client = getDBClient();
    const char armedSubcategory[] = "co";

    auto initDeviceItems = [this, &armedSubcategory]( const char* deviceName ) {
        auto& client = getDBClient();
        if( !deviceName || !*deviceName )
            throw std::runtime_error( "Empty device name" );

        {
            Json device = v1_37::Resources::vistaCamMotionSensor;
            device[ Fields::id ] = deviceName;
            device[ Fields::subcategory ] = armedSubcategory;
            device[ Fields::name ] = deviceName;
            device[ Fields::armed ] = true;
            client.insert( Tables::DevicesDb, device );
        }

        {
            Json device = v1_37::Resources::vistaCamMotionSensor;
            device[ Fields::id ] = std::string( deviceName ) + "1";
            device[ Fields::subcategory ] = armedSubcategory;
            device[ Fields::name ] = deviceName;
            device[ Fields::armed ] = false;
            client.insert( Tables::DevicesDb, device );
        }

        {
            Json device = v1_37::Resources::vistaCamMotionSensor;
            const std::string& deviceId = std::string( deviceName ) + "2";
            device[ Fields::id ] = deviceId;
            device[ Fields::subcategory ] = armedSubcategory;
            device[ Fields::name ] = deviceName;
            device[ Fields::armed ] = true;
            client.insert( Tables::DevicesDb, device );

            Json item = { { Fields::id, "security_threat_id1" },
                          { Fields::name, Values::securityThreat },
                          { Fields::hasSetter, false },
                          { Fields::hasGetter, true },
                          { Fields::show, true },
                          { Fields::value, 0 },
                          { Fields::type, "int" },
                          { Fields::deviceId, deviceId },
                          { Fields::enumField, JSON_EMPTY_ARR } };
            client.insert( Tables::ItemsDb, item );
        }
    };

    const char* deviceName = "security_sensor";
    initDeviceItems( deviceName );

    MigrationTool::v1_39_security_threat::Migration migration( client );
    ASSERT_TRUE( migration.migrate() ) << "Migration should return 'true'";
    ASSERT_EQ( migration.modifiedDevices(), 1 ) << "One device should be modified";
}

}  // namespace UT
}  // namespace HubPlatform
