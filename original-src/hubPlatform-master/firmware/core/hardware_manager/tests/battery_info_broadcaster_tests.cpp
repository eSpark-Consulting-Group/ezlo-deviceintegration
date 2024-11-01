#include "hardware_manager/battery/battery_info_broadcaster.h"
#include "hardware_manager/battery/battery_info.h"
#include "database_client/fake_db/fake_db_client.h"
#include "common_utils/cpp14.h"
#include <gtest/gtest.h>

namespace HubPlatform {
namespace HardwareManager {

namespace UT {

class BatteryInfoBroadcasterTests : public ::testing::Test {
public:
    IDbClient& getDBClient() {
        return m_dbClient;
    }
    void SetUp() override {
        m_modifierUPtr = std::make_unique< BatteryInfoBroadcaster >( getDBClient(), [this]( const Json& broadcast ) {
            LOGGER_TRACE_RAW( "Broadcast callback called: " + cc::j( broadcast ) );
            if( json_get_bool( broadcast, "userNotification", false ) ) {
                m_userNotified++;
            }
        } );
    }
    void TearDown() override {
        m_dbClient.clean();
        m_modifierUPtr.reset();
        m_userNotified = 0;
        ::testing::Test::TearDown();
    }

    void handleBatteryInfo( const BatteryInfo& info ) {
        m_modifierUPtr->makeBroadcastAndSend( info );
    }

protected:
    ::HubPlatform::UT::FakeDbClient m_dbClient;
    std::unique_ptr< BatteryInfoBroadcaster > m_modifierUPtr;
    unsigned m_userNotified = 0;
};

TEST_F( BatteryInfoBroadcasterTests, dischargingToCharging ) {
    BatteryInfo info0, info2, info3, info4, info5;
    info0 = BatteryInfo::fromJson( { { "status", "initializing" } } );
    handleBatteryInfo( info0 );
    EXPECT_EQ( m_userNotified, 0 );
    info2 = BatteryInfo::fromJson( { { "status", "discharging" } } );
    handleBatteryInfo( info2 );
    EXPECT_EQ( m_userNotified, 1 );
    info3 = BatteryInfo::fromJson( { { "status", "charging" }, { "stateOfCharge", 50 } } );
    handleBatteryInfo( info3 );
    EXPECT_EQ( m_userNotified, 2 );
    info4 = BatteryInfo::fromJson( { { "status", "full" }, { "stateOfCharge", 100 } } );
    handleBatteryInfo( info4 );
    EXPECT_EQ( m_userNotified, 2 );
    info5 = BatteryInfo::fromJson( { { "status", "discharging" }, { "stateOfCharge", 99 } } );
    handleBatteryInfo( info5 );
    EXPECT_EQ( m_userNotified, 3 );
}

TEST_F( BatteryInfoBroadcasterTests, statusDischarging_plugACThenRemoveBatteryRightAfter ) {
    BatteryInfo info0, info2, info3;
    info0 = BatteryInfo::fromJson( { { "status", "initializing" } } );
    handleBatteryInfo( info0 );
    EXPECT_EQ( m_userNotified, 0 );
    info2 = BatteryInfo::fromJson( { { "status", "discharging" } } );
    handleBatteryInfo( info2 );
    EXPECT_EQ( m_userNotified, 1 );
    info3 = BatteryInfo::fromJson( { { "status", "missing" } } );
    handleBatteryInfo( info3 );
    EXPECT_EQ( m_userNotified, 2 );
}

TEST_F( BatteryInfoBroadcasterTests, statusFull_plugACThenRemoveBatteryRightAfter ) {
    BatteryInfo info0, info1, info2, info3;
    info0 = BatteryInfo::fromJson( { { "status", "initializing" } } );
    handleBatteryInfo( info0 );
    EXPECT_EQ( m_userNotified, 0 );
    info1 = BatteryInfo::fromJson( { { "status", "charging" }, { "stateOfCharge", 99 } } );
    handleBatteryInfo( info1 );
    info2 = BatteryInfo::fromJson( { { "status", "full" }, { "stateOfCharge", 100 } } );
    handleBatteryInfo( info2 );
    info3 = BatteryInfo::fromJson( { { "status", "missing" } } );
    handleBatteryInfo( info3 );
    EXPECT_EQ( m_userNotified, 1 );
}

TEST_F( BatteryInfoBroadcasterTests, repeatDischarging ) {
    handleBatteryInfo( BatteryInfo::fromJson( { { "status", "initializing" } } ) );
    EXPECT_EQ( m_userNotified, 0 );
    handleBatteryInfo( BatteryInfo::fromJson( { { "status", "charging" } } ) );
    handleBatteryInfo( BatteryInfo::fromJson( { { "status", "discharging" } } ) );
    handleBatteryInfo( BatteryInfo::fromJson( { { "status", "discharging" } } ) );
    EXPECT_EQ( m_userNotified, 1 );
}

TEST_F( BatteryInfoBroadcasterTests, dischargingInitCharging ) {
    handleBatteryInfo( BatteryInfo::fromJson( { { "status", "discharging" } } ) );
    handleBatteryInfo( BatteryInfo::fromJson( { { "status", "initializing" } } ) );
    handleBatteryInfo( BatteryInfo::fromJson( { { "status", "charging" } } ) );
    EXPECT_EQ( m_userNotified, 1 );
}

TEST_F( BatteryInfoBroadcasterTests, dischargingInitFull ) {
    handleBatteryInfo( BatteryInfo::fromJson( { { "status", "discharging" } } ) );
    handleBatteryInfo( BatteryInfo::fromJson( { { "status", "initializing" } } ) );
    handleBatteryInfo( BatteryInfo::fromJson( { { "status", "full" } } ) );
    EXPECT_EQ( m_userNotified, 1 );
}

}  // namespace UT

}  // namespace HardwareManager
}  // namespace HubPlatform
