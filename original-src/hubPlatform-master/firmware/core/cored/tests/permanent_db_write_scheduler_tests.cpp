#include "cored/tests/service/test_core_service.h"
#include "cored/tests/fakes/fake_feature_manager.h"

#include "cored/core_service.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/permanent_db_write_scheduler.h"
#include "cored/db_modifier/device_db_modifier.h"
#include "cored/db_modifier/item_db_modifier.h"
#include "cored/swinger_shutdown/swinger_shutdown.h"
#include "cored/db_modifier/security_item_db_modifier.h"

#include "service_interfaces/tests/fixtures/fake_service_fixture.h"
#include "cored/house_modes/resources/house_mode_resources.h"
#include "common_utils/cpp14.h"

#include <memory>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace HubPlatform {
namespace UT {

using namespace Services;

class DbWriterTest : public Services::UT::FakeServiceTestFixture {
public:
    DbWriterTest()
    : m_featureManager( std::make_unique< CoreD::UT::FakeFeatureManager >() )
    , m_deviceDbModifier( getDBClient(), []( MessageSubClasses, const Json& ) {} )
    , m_swingerShutdown( getHostService() )
    , m_securityItemDbModifier( getHostService(), *m_featureManager, m_swingerShutdown )
    , m_itemDbModifier( getHostService(),
                        m_provisionData,
                        []( MessageSubClasses, const Json& ) {},
                        *m_featureManager,
                        m_securityItemDbModifier )
    , m_permanentDbWriterScheduler( getHostService(), m_itemDbModifier, m_deviceDbModifier ) {}

    void SetUp() override {
        cleanDB();
    }

    void TearDown() override {
        cleanDB();
    }

    IDbClient& getDBClient() {
        return getHostService().getDBClient();
    }

    CoreD::DeviceDbModifier& getDeviceDbModifier() {
        return m_deviceDbModifier;
    }

    CoreD::ItemDbModifier& getItemDbModifier() {
        return m_itemDbModifier;
    }

    void cleanDB() {
        getDBClient().clean();
    }

    CoreD::PermanentDBWriteScheduler& getWriter() {
        return m_permanentDbWriterScheduler;
    }

private:
    std::unique_ptr< CoreD::IFeaturesManager > m_featureManager;
    CoreD::DeviceDbModifier m_deviceDbModifier;
    CoreD::SwingerShutdown::SwingerShutdown m_swingerShutdown;
    CoreD::SecurityItemDbModifier m_securityItemDbModifier;
    CoreD::ItemDbModifier m_itemDbModifier;
    CoreD::PermanentDBWriteScheduler m_permanentDbWriterScheduler;
    ProvisionDataUtils::ProvisionData m_provisionData;
};

TEST_F( DbWriterTest, writeToPermanentDb ) {
    // Steps:
    // Create 1 device and 1 item.
    // Update value of item and reachable state of device via db modifieres.
    // Check that values are updated only in temp db.
    // Call 'doNow()' for writer
    // Check that values are updated in the permanent db.
    using namespace CoreD::Model::Resources;
    getDeviceDbModifier().insert(
        JSON( DeviceFields::IdFieldName << "deviceId_1" << DeviceFields::ReachableFieldName << true ) );

    EXPECT_EQ( getDBClient().count( DBNames::DevicesDB, JSON_EMPTY_OBJ ), 1 );
    EXPECT_EQ( getDBClient().count( DBNames::TempDevicesDB, JSON_EMPTY_OBJ ), 1 );

    getItemDbModifier().insert(
        JSON( ItemFields::IdFieldName << "itemId_1" << ItemFields::ValueFieldName << "initial value" ) );

    {
        const auto result =
            getDBClient().findOne( DBNames::ItemsDB, JSON( ItemFields::IdFieldName << "itemId_1" ), nullptr );
        EXPECT_EQ( "initial value", json_get_str( result, ItemFields::ValueFieldName ) );
    }
    {
        const auto result =
            getDBClient().findOne( DBNames::TempItemsDB, JSON( ItemFields::IdFieldName << "itemId_1" ), nullptr );
        EXPECT_EQ( "initial value", json_get_str( result, ItemFields::ValueFieldName ) );
    }

    getItemDbModifier().setChangedStatus( false );
    getItemDbModifier().update( "itemId_1", JSON( ItemFields::ValueFieldName << "new value" ) );
    EXPECT_TRUE( getItemDbModifier().hasChanges() );

    {
        const auto result =
            getDBClient().findOne( DBNames::ItemsDB, JSON( ItemFields::IdFieldName << "itemId_1" ), nullptr );
        EXPECT_EQ( "initial value", json_get_str( result, ItemFields::ValueFieldName ) );
    }
    {
        const auto result =
            getDBClient().findOne( DBNames::TempItemsDB, JSON( ItemFields::IdFieldName << "itemId_1" ), nullptr );
        EXPECT_EQ( "new value", json_get_str( result, ItemFields::ValueFieldName ) );
    }

    getDeviceDbModifier().setChangedStatus( false );
    getDeviceDbModifier().update( "deviceId_1", JSON( DeviceFields::ReachableFieldName << false ) );
    EXPECT_TRUE( getDeviceDbModifier().hasChanges() );

    {
        const auto result =
            getDBClient().findOne( DBNames::DevicesDB, JSON( DeviceFields::IdFieldName << "deviceId_1" ), nullptr );
        EXPECT_EQ( true, json_get_bool( result, DeviceFields::ReachableFieldName ) );
    }
    {
        const auto result =
            getDBClient().findOne( DBNames::TempDevicesDB, JSON( DeviceFields::IdFieldName << "deviceId_1" ), nullptr );
        EXPECT_EQ( false, json_get_bool( result, DeviceFields::ReachableFieldName ) );
    }

    getWriter().doNow();
    EXPECT_FALSE( getItemDbModifier().hasChanges() );
    EXPECT_FALSE( getDeviceDbModifier().hasChanges() );

    {
        const auto result =
            getDBClient().findOne( DBNames::ItemsDB, JSON( ItemFields::IdFieldName << "itemId_1" ), nullptr );
        EXPECT_EQ( "new value", json_get_str( result, ItemFields::ValueFieldName ) );
    }
    {
        const auto result =
            getDBClient().findOne( DBNames::TempItemsDB, JSON( ItemFields::IdFieldName << "itemId_1" ), nullptr );
        EXPECT_EQ( "new value", json_get_str( result, ItemFields::ValueFieldName ) );
    }

    {
        const auto result =
            getDBClient().findOne( DBNames::DevicesDB, JSON( DeviceFields::IdFieldName << "deviceId_1" ), nullptr );
        EXPECT_EQ( false, json_get_bool( result, DeviceFields::ReachableFieldName ) );
    }
    {
        const auto result =
            getDBClient().findOne( DBNames::TempDevicesDB, JSON( DeviceFields::IdFieldName << "deviceId_1" ), nullptr );
        EXPECT_EQ( false, json_get_bool( result, DeviceFields::ReachableFieldName ) );
    }
}

TEST_F( DbWriterTest, commandTimestampsToPermanentDb ) {
    const auto oldValue = std::chrono::milliseconds( 200 );
    const auto newValue = std::chrono::milliseconds( 400 );
    using namespace CoreDHouseModes::Resources;

    {
        getDBClient().insert( DBNames::HouseModesCommandTimestampsDBName,
                              JSON( HouseModeCommandFields::CommandFieldName
                                    << "hub.modes.protect.set" << HouseModeCommandFields::ModeIdFieldName << "1"
                                    << HouseModeCommandFields::TimestampFieldName << oldValue.count() ) );

        EXPECT_EQ( getDBClient().count( DBNames::HouseModesCommandTimestampsDBName, JSON_EMPTY_OBJ ), 1 );
    }

    {
        getDBClient().insert( HaTempHouseModesCommandTimestampsDBName,
                              JSON( HouseModeCommandFields::CommandFieldName
                                    << "hub.modes.protect.set" << HouseModeCommandFields::ModeIdFieldName << "1"
                                    << HouseModeCommandFields::TimestampFieldName << oldValue.count() ) );
        EXPECT_EQ( getDBClient().count( HaTempHouseModesCommandTimestampsDBName, JSON_EMPTY_OBJ ), 1 );
    }

    {
        getDBClient().update(
            HaTempHouseModesCommandTimestampsDBName,
            JSON( HouseModeCommandFields::CommandFieldName << "hub.modes.protect.set"
                                                           << HouseModeCommandFields::ModeIdFieldName << "1" ),
            JSON( "$set" << JSON( HouseModeCommandFields::TimestampFieldName << newValue.count() ) ) );
    }

    {
        const auto result =
            getDBClient().findOne( DBNames::HouseModesCommandTimestampsDBName,
                                   JSON( HouseModeCommandFields::CommandFieldName
                                         << "hub.modes.protect.set" << HouseModeCommandFields::ModeIdFieldName << "1" ),
                                   nullptr );
        EXPECT_EQ( oldValue.count(), json_get_int64( result, HouseModeCommandFields::TimestampFieldName ) );
    }

    {
        const auto result =
            getDBClient().findOne( HaTempHouseModesCommandTimestampsDBName,
                                   JSON( HouseModeCommandFields::CommandFieldName
                                         << "hub.modes.protect.set" << HouseModeCommandFields::ModeIdFieldName << "1" ),
                                   nullptr );
        EXPECT_EQ( newValue.count(), json_get_int64( result, HouseModeCommandFields::TimestampFieldName ) );
    }
    getWriter().doNow();
    {
        const auto result =
            getDBClient().findOne( DBNames::HouseModesCommandTimestampsDBName,
                                   JSON( HouseModeCommandFields::CommandFieldName
                                         << "hub.modes.protect.set" << HouseModeCommandFields::ModeIdFieldName << "1" ),
                                   nullptr );
        EXPECT_EQ( newValue.count(), json_get_int64( result, HouseModeCommandFields::TimestampFieldName ) );
    }
}

}  // namespace UT
}  // namespace HubPlatform
