#include "update_manager/um_update_planner.h"
#include "update_manager/common/um_update_operation.h"

#include "log_utils/ezlo_log.h"
#include "json_utils/json_utils.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

/*

TESTPLAN

DONE    1 Upgrade firmware
DONE        1.1 Same system, Same firmware, same extensions
DONE        1.2 Same system, Same firmware, new extensions
DONE        1.3 Same system, Same firmware, old extensions
DONE        1.4 Same system, New firmware, same extensions
DONE        1.5 Same system, New firmware, new extensions
DONE        1.6 Old firmware
DONE        1.7 System version
DONE            1.7.1 Current missing
DONE            1.7.2 Requested missing
DONE            1.7.3 Differ upgrade
DONE            1.7.4 Differ sysupgrade
DONE        1.8 Explicitly specified source
DONE            1.8.1 Upgrade
DONE            1.8.2 Sysupgrade
DONE        1.9 Sysupgrade for no reason means upgrade (reverted)
DONE        1.10 Sysupgrade same firmware, same extensions
DONE    2 Upgrade extension
DONE        2.1 Same version
DONE        2.2 New version
DONE        2.3 Old version
DONE    3 Install extension
DONE        3.1 Same snapshot
DONE        3.2 New snapshot
DONE        3.3 Old snapshot
DONE    4 Uninstall extension
DONE    5 Special cases
DONE        5.1 Missing package at new snapshot
DONE        5.2 Custom plugins validator: don't touch other type's actions
DONE        5.3 Custom plugins validator: check custom_plugin actions
DONE    6 Update operation cases
DONE        6.1 Check serialization & deserialization
DONE        6.2 Check invalid operation type deserialization
DONE        6.3 Check invalid type deserialization
*/

using namespace ::testing;
using namespace HubPlatform;
using namespace HubPlatform::UpdateManager;

class MockPackagesManager : public Opkg::IPackagesManager {
public:
    MOCK_CONST_METHOD0( getInstalledPackages, Opkg::IPackagesManager::PackageSummaries() );
    MOCK_CONST_METHOD0( getUpgradablePackages, Opkg::IPackagesManager::UpgradablePackageSummaries() );
    MOCK_CONST_METHOD0( getAvailablePackages, Opkg::IPackagesManager::PackageSummaries() );
    MOCK_CONST_METHOD1( getDependantPackages, Opkg::IPackagesManager::PackageSummaries( const std::string& ) );

    MOCK_METHOD2( downloadImage, StdOutWithReturnCode( const std::string&, const std::string& ) );
    MOCK_METHOD3( downloadPackage, StdOutWithReturnCode( const std::string&, const std::string&, bool ) );

    MOCK_METHOD0( updateAvailable, StdOutWithReturnCode() );
    MOCK_METHOD0( checkAvailable, StdOutWithReturnCode() );
    MOCK_METHOD2( sysupgrade, StdOutWithReturnCode( const std::string&, const std::string& ) );
    MOCK_METHOD2( upgradePackage, StdOutWithReturnCode( const std::string&, const std::string& ) );
    MOCK_METHOD2( installPackage, StdOutWithReturnCode( const std::string&, const std::string& ) );
    MOCK_METHOD1( removePackage, StdOutWithReturnCode( const std::string& ) );
};

class MockFeedsUpdater : public Opkg::IFeedsUpdater {
public:
    MOCK_METHOD2( update, bool( const std::string&, const int ) );
    MOCK_METHOD1( update, void( const std::string& ) );

    MOCK_METHOD0( resetCurrent, void() );
    MOCK_METHOD0( resetToCurrent, void() );

    MOCK_CONST_METHOD0( getFeedsSource, std::string() );

    MOCK_CONST_METHOD0( getFeedsUpdateVersion, std::string() );
    MOCK_CONST_METHOD0( getExpectedUpdateVersion, std::string() );

    MOCK_CONST_METHOD0( getFeedsSnapshot, int() );
    MOCK_CONST_METHOD0( generateImageUrl, std::string() );
    MOCK_CONST_METHOD0( generateRecoveryImageUrl, std::string() );
};

class UpdatePlannerTestSuite : public Test {
public:
    UpdatePlannerTestSuite() {
        // Do not print errors and warnings on erroneous test cases
        // TODO: implement fake log and check error messages too.
        //        Log::setLocalLogLevel( Log::FATAL );
    }

    UpdatePlanner createUpdatePlanner( const std::string& firmwareVersion ) {
        UpdatePlanner planner( m_packagesManager, m_feedsUpdater, Firmware::Version( firmwareVersion ) );
        return planner;
    }

protected:
    MockPackagesManager m_packagesManager;
    MockFeedsUpdater m_feedsUpdater;
};

TEST_F( UpdatePlannerTestSuite, testUpgradeFirmwareSameSystemSameFirmwareSameExtensions /*1.1*/ ) {
    EXPECT_CALL( m_packagesManager, getInstalledPackages() )
        .WillOnce( Return( Opkg::IPackagesManager::PackageSummaries( { { "hub-platform-system", "1.0.76" },
                                                                       { "hub-platform-firmware", "2.0.1.24" },
                                                                       { "firmware-addons-cameras", "1.0.24" } } ) ) );

    EXPECT_CALL( m_packagesManager, getAvailablePackages() )
        .WillOnce( Return( Opkg::IPackagesManager::PackageSummaries( { { "hub-platform-system", "1.0.76" },
                                                                       { "hub-platform-firmware", "2.0.1.24" },
                                                                       { "firmware-addons-cameras", "1.0.24" } } ) ) );

    EXPECT_CALL( m_feedsUpdater, getFeedsSource() ).WillOnce( Return( "http://dl.mios.com/linux_firmware/qa" ) );
    EXPECT_CALL( m_feedsUpdater, getFeedsSnapshot() ).WillOnce( Return( 24 ) );
    UpdateOperation operation = UpdateOperation::fromJson( R"({
        "operation": "upgrade",
        "type": "firmware"
})"_json );

    std::string expectedPlan = R"({
    "actions": []
})";

    auto planner = createUpdatePlanner( "2.0.1.24" );
    auto plan = planner.createUpdatePlan( operation );
    EXPECT_EQ( expectedPlan, plan->toJson().dump( 4 ) );

    std::string errorMessage, errorField;
    ASSERT_TRUE( UpdatePlan::validateCustomPluginActions( plan->toJson(), errorMessage, errorField ) );
}

TEST_F( UpdatePlannerTestSuite, testUpgradeFirmwareSameSystemSameFirmwareNewExtensions /*1.2*/ ) {
    EXPECT_CALL( m_packagesManager, getInstalledPackages() )
        .WillOnce( Return( Opkg::IPackagesManager::PackageSummaries( { { "hub-platform-system", "1.0.76" },
                                                                       { "hub-platform-firmware", "2.0.1.24" },
                                                                       { "firmware-addons-cameras", "1.0.24" } } ) ) );

    EXPECT_CALL( m_packagesManager, getAvailablePackages() )
        .WillOnce( Return( Opkg::IPackagesManager::PackageSummaries( { { "hub-platform-system", "1.0.76" },
                                                                       { "hub-platform-firmware", "2.0.1.24" },
                                                                       { "firmware-addons-cameras", "1.0.25" } } ) ) );

    EXPECT_CALL( m_feedsUpdater, getFeedsSource() ).WillOnce( Return( "http://dl.mios.com/linux_firmware/qa" ) );
    EXPECT_CALL( m_feedsUpdater, getFeedsSnapshot() ).WillOnce( Return( 25 ) );
    UpdateOperation operation = UpdateOperation::fromJson( R"({
        "operation": "upgrade",
        "type": "firmware"
})"_json );

    std::string expectedPlan = R"({
    "actions": [
        {
            "id": "cameras",
            "operation": "upgrade",
            "snapshot": 25,
            "source": "http://dl.mios.com/linux_firmware/qa",
            "type": "addon",
            "version": "1.0.25"
        }
    ]
})";

    auto planner = createUpdatePlanner( "2.0.1.24" );
    auto plan = planner.createUpdatePlan( operation );
    EXPECT_EQ( expectedPlan, plan->toJson().dump( 4 ) );

    std::string errorMessage, errorField;
    ASSERT_TRUE( UpdatePlan::validateCustomPluginActions( plan->toJson(), errorMessage, errorField ) )
        << "message: " << errorMessage << ", field: " << errorField;
}

TEST_F( UpdatePlannerTestSuite, testUpgradeFirmwareSameSystemSameFirmwareOldExtensions /*1.3*/ ) {
    EXPECT_CALL( m_packagesManager, getInstalledPackages() )
        .WillOnce( Return( Opkg::IPackagesManager::PackageSummaries( { { "hub-platform-system", "1.0.76" },
                                                                       { "hub-platform-firmware", "2.0.1.24" },
                                                                       { "firmware-addons-cameras", "1.0.24" } } ) ) );

    EXPECT_CALL( m_packagesManager, getAvailablePackages() )
        .WillOnce( Return( Opkg::IPackagesManager::PackageSummaries( { { "hub-platform-system", "1.0.76" },
                                                                       { "hub-platform-firmware", "2.0.1.24" },
                                                                       { "firmware-addons-cameras", "1.0.23" } } ) ) );

    EXPECT_CALL( m_feedsUpdater, getFeedsSource() ).WillOnce( Return( "http://dl.mios.com/linux_firmware/qa" ) );
    EXPECT_CALL( m_feedsUpdater, getFeedsSnapshot() ).WillOnce( Return( 25 ) );
    UpdateOperation operation = UpdateOperation::fromJson( R"({
        "operation": "upgrade",
        "type": "firmware"
})"_json );

    std::string expectedPlan = R"({
    "actions": [
        {
            "operation": "sysupgrade",
            "snapshot": 25,
            "source": "http://dl.mios.com/linux_firmware/qa",
            "type": "firmware",
            "version": "2.0.1.24"
        },
        {
            "id": "cameras",
            "operation": "install",
            "snapshot": 25,
            "source": "http://dl.mios.com/linux_firmware/qa",
            "type": "addon",
            "version": "1.0.23"
        }
    ]
})";

    auto planner = createUpdatePlanner( "2.0.1.24" );
    auto plan = planner.createUpdatePlan( operation );
    EXPECT_EQ( expectedPlan, plan->toJson().dump( 4 ) );
}

TEST_F( UpdatePlannerTestSuite, testUpgradeFirmwareSameSystemNewFirmwareSameExtensions /*1.4*/ ) {
    EXPECT_CALL( m_packagesManager, getInstalledPackages() )
        .WillOnce( Return( Opkg::IPackagesManager::PackageSummaries( { { "hub-platform-system", "1.0.76" },
                                                                       { "hub-platform-firmware", "2.0.1.24" },
                                                                       { "firmware-addons-cameras", "1.0.24" } } ) ) );

    EXPECT_CALL( m_packagesManager, getAvailablePackages() )
        .WillOnce( Return( Opkg::IPackagesManager::PackageSummaries( { { "hub-platform-system", "1.0.76" },
                                                                       { "hub-platform-firmware", "2.0.1.25" },
                                                                       { "firmware-addons-cameras", "1.0.24" } } ) ) );

    EXPECT_CALL( m_feedsUpdater, getFeedsSource() ).WillOnce( Return( "http://dl.mios.com/linux_firmware/qa" ) );
    EXPECT_CALL( m_feedsUpdater, getFeedsSnapshot() ).WillOnce( Return( 25 ) );
    UpdateOperation operation = UpdateOperation::fromJson( R"({
        "operation": "upgrade",
        "type": "firmware"
})"_json );

    std::string expectedPlan = R"({
    "actions": [
        {
            "operation": "upgrade",
            "snapshot": 25,
            "source": "http://dl.mios.com/linux_firmware/qa",
            "type": "firmware",
            "version": "2.0.1.25"
        }
    ]
})";

    auto planner = createUpdatePlanner( "2.0.1.24" );
    auto plan = planner.createUpdatePlan( operation );
    EXPECT_EQ( expectedPlan, plan->toJson().dump( 4 ) );
}

TEST_F( UpdatePlannerTestSuite, testUpgradeFirmwareSameSystemNewFirmwareNewExtensions /*1.5*/ ) {
    EXPECT_CALL( m_packagesManager, getInstalledPackages() )
        .WillOnce( Return( Opkg::IPackagesManager::PackageSummaries( { { "hub-platform-system", "1.0.76" },
                                                                       { "hub-platform-firmware", "2.0.1.24" },
                                                                       { "firmware-addons-cameras", "1.0.24" } } ) ) );

    EXPECT_CALL( m_packagesManager, getAvailablePackages() )
        .WillOnce( Return( Opkg::IPackagesManager::PackageSummaries( { { "hub-platform-system", "1.0.76" },
                                                                       { "hub-platform-firmware", "2.0.1.25" },
                                                                       { "firmware-addons-cameras", "1.0.25" } } ) ) );

    EXPECT_CALL( m_feedsUpdater, getFeedsSource() ).WillOnce( Return( "http://dl.mios.com/linux_firmware/qa" ) );
    EXPECT_CALL( m_feedsUpdater, getFeedsSnapshot() ).WillOnce( Return( 25 ) );
    UpdateOperation operation = UpdateOperation::fromJson( R"({
        "operation": "upgrade",
        "type": "firmware"
})"_json );

    std::string expectedPlan = R"({
    "actions": [
        {
            "operation": "upgrade",
            "snapshot": 25,
            "source": "http://dl.mios.com/linux_firmware/qa",
            "type": "firmware",
            "version": "2.0.1.25"
        },
        {
            "id": "cameras",
            "operation": "upgrade",
            "snapshot": 25,
            "source": "http://dl.mios.com/linux_firmware/qa",
            "type": "addon",
            "version": "1.0.25"
        }
    ]
})";

    auto planner = createUpdatePlanner( "2.0.1.24" );
    auto plan = planner.createUpdatePlan( operation );
    EXPECT_EQ( expectedPlan, plan->toJson().dump( 4 ) );
}

TEST_F( UpdatePlannerTestSuite, testUpgradeFirmwareOldFirmware /*1.6*/ ) {
    EXPECT_CALL( m_packagesManager, getInstalledPackages() )
        .WillOnce( Return( Opkg::IPackagesManager::PackageSummaries( { { "hub-platform-system", "1.0.76" },
                                                                       { "hub-platform-firmware", "2.0.1.24" },
                                                                       { "firmware-addons-cameras", "1.0.24" } } ) ) );

    EXPECT_CALL( m_packagesManager, getAvailablePackages() )
        .WillOnce( Return( Opkg::IPackagesManager::PackageSummaries( { { "hub-platform-system", "1.0.76" },
                                                                       { "hub-platform-firmware", "2.0.1.23" },
                                                                       { "firmware-addons-cameras", "1.0.24" } } ) ) );

    EXPECT_CALL( m_feedsUpdater, getFeedsSource() ).WillOnce( Return( "http://dl.mios.com/linux_firmware/qa" ) );
    EXPECT_CALL( m_feedsUpdater, getFeedsSnapshot() ).WillOnce( Return( 25 ) );
    UpdateOperation operation = UpdateOperation::fromJson( R"({
        "operation": "upgrade",
        "type": "firmware"
})"_json );

    std::string expectedPlan = R"({
    "actions": [
        {
            "operation": "sysupgrade",
            "snapshot": 25,
            "source": "http://dl.mios.com/linux_firmware/qa",
            "type": "firmware",
            "version": "2.0.1.23"
        },
        {
            "id": "cameras",
            "operation": "install",
            "snapshot": 25,
            "source": "http://dl.mios.com/linux_firmware/qa",
            "type": "addon",
            "version": "1.0.24"
        }
    ]
})";

    auto planner = createUpdatePlanner( "2.0.1.24" );
    auto plan = planner.createUpdatePlan( operation );
    EXPECT_EQ( expectedPlan, plan->toJson().dump( 4 ) );
}

TEST_F( UpdatePlannerTestSuite, testUpgradeFirmwareCurrentSystemVersionMissing /*1.7.1*/ ) {
    EXPECT_CALL( m_packagesManager, getInstalledPackages() )
        .WillOnce( Return( Opkg::IPackagesManager::PackageSummaries(
            { { "hub-platform-firmware", "2.0.1.24" }, { "firmware-addons-cameras", "1.0.24" } } ) ) );

    EXPECT_CALL( m_packagesManager, getAvailablePackages() )
        .WillOnce( Return( Opkg::IPackagesManager::PackageSummaries( { { "hub-platform-system", "1.0.76" },
                                                                       { "hub-platform-firmware", "2.0.1.24" },
                                                                       { "firmware-addons-cameras", "1.0.24" } } ) ) );

    EXPECT_CALL( m_feedsUpdater, getFeedsSource() ).WillOnce( Return( "http://dl.mios.com/linux_firmware/qa" ) );
    EXPECT_CALL( m_feedsUpdater, getFeedsSnapshot() ).WillOnce( Return( 25 ) );
    UpdateOperation operation = UpdateOperation::fromJson( R"({
        "operation": "upgrade",
        "type": "firmware"
})"_json );

    std::string expectedPlan = R"({
    "actions": [
        {
            "operation": "sysupgrade",
            "snapshot": 25,
            "source": "http://dl.mios.com/linux_firmware/qa",
            "type": "firmware",
            "version": "2.0.1.24"
        },
        {
            "id": "cameras",
            "operation": "install",
            "snapshot": 25,
            "source": "http://dl.mios.com/linux_firmware/qa",
            "type": "addon",
            "version": "1.0.24"
        }
    ]
})";

    auto planner = createUpdatePlanner( "2.0.1.24" );
    auto plan = planner.createUpdatePlan( operation );
    EXPECT_EQ( expectedPlan, plan->toJson().dump( 4 ) );
}

TEST_F( UpdatePlannerTestSuite, testUpgradeFirmwareRequestedSystemVersionMissing /*1.7.2*/ ) {
    EXPECT_CALL( m_packagesManager, getInstalledPackages() )
        .WillOnce( Return( Opkg::IPackagesManager::PackageSummaries( { { "hub-platform-system", "1.0.76" },
                                                                       { "hub-platform-firmware", "2.0.1.24" },
                                                                       { "firmware-addons-cameras", "1.0.24" } } ) ) );

    EXPECT_CALL( m_packagesManager, getAvailablePackages() )
        .WillOnce( Return( Opkg::IPackagesManager::PackageSummaries(
            { { "hub-platform-firmware", "2.0.1.24" }, { "firmware-addons-cameras", "1.0.24" } } ) ) );

    EXPECT_CALL( m_feedsUpdater, getFeedsSource() ).WillOnce( Return( "http://dl.mios.com/linux_firmware/qa" ) );
    EXPECT_CALL( m_feedsUpdater, getFeedsSnapshot() ).WillOnce( Return( 25 ) );
    UpdateOperation operation = UpdateOperation::fromJson( R"({
        "operation": "upgrade",
        "type": "firmware"
})"_json );

    std::string expectedPlan = R"({
    "actions": [
        {
            "operation": "sysupgrade",
            "snapshot": 25,
            "source": "http://dl.mios.com/linux_firmware/qa",
            "type": "firmware",
            "version": "2.0.1.24"
        },
        {
            "id": "cameras",
            "operation": "install",
            "snapshot": 25,
            "source": "http://dl.mios.com/linux_firmware/qa",
            "type": "addon",
            "version": "1.0.24"
        }
    ]
})";

    auto planner = createUpdatePlanner( "2.0.1.24" );
    auto plan = planner.createUpdatePlan( operation );
    EXPECT_EQ( expectedPlan, plan->toJson().dump( 4 ) );
}

TEST_F( UpdatePlannerTestSuite, testUpgradeFirmwareDifferentSystemVersionUpgrade /*1.7.3*/ ) {
    EXPECT_CALL( m_packagesManager, getInstalledPackages() )
        .WillOnce( Return( Opkg::IPackagesManager::PackageSummaries( { { "hub-platform-system", "1.0.76" },
                                                                       { "hub-platform-firmware", "2.0.1.24" },
                                                                       { "firmware-addons-cameras", "1.0.24" } } ) ) );

    EXPECT_CALL( m_packagesManager, getAvailablePackages() )
        .WillOnce( Return( Opkg::IPackagesManager::PackageSummaries( { { "hub-platform-system", "1.0.94" },
                                                                       { "hub-platform-firmware", "2.0.1.24" },
                                                                       { "firmware-addons-cameras", "1.0.24" } } ) ) );

    EXPECT_CALL( m_feedsUpdater, getFeedsSource() ).WillOnce( Return( "http://dl.mios.com/linux_firmware/qa" ) );
    EXPECT_CALL( m_feedsUpdater, getFeedsSnapshot() ).WillOnce( Return( 25 ) );
    UpdateOperation operation = UpdateOperation::fromJson( R"({
        "operation": "upgrade",
        "type": "firmware"
})"_json );

    std::string expectedPlan = R"({
    "actions": [
        {
            "operation": "sysupgrade",
            "snapshot": 25,
            "source": "http://dl.mios.com/linux_firmware/qa",
            "type": "firmware",
            "version": "2.0.1.24"
        },
        {
            "id": "cameras",
            "operation": "install",
            "snapshot": 25,
            "source": "http://dl.mios.com/linux_firmware/qa",
            "type": "addon",
            "version": "1.0.24"
        }
    ]
})";

    auto planner = createUpdatePlanner( "2.0.1.24" );
    auto plan = planner.createUpdatePlan( operation );
    EXPECT_EQ( expectedPlan, plan->toJson().dump( 4 ) );
}

TEST_F( UpdatePlannerTestSuite, testUpgradeFirmwareDifferentSystemVersionSysupgrade /*1.7.4*/ ) {
    EXPECT_CALL( m_packagesManager, getInstalledPackages() )
        .WillOnce( Return( Opkg::IPackagesManager::PackageSummaries( { { "hub-platform-system", "1.0.76" },
                                                                       { "hub-platform-firmware", "2.0.1.24" },
                                                                       { "firmware-addons-cameras", "1.0.24" } } ) ) );

    EXPECT_CALL( m_packagesManager, getAvailablePackages() )
        .WillOnce( Return( Opkg::IPackagesManager::PackageSummaries( { { "hub-platform-system", "1.0.94" },
                                                                       { "hub-platform-firmware", "2.0.1.24" },
                                                                       { "firmware-addons-cameras", "1.0.24" } } ) ) );

    EXPECT_CALL( m_feedsUpdater, getFeedsSource() ).WillOnce( Return( "http://dl.mios.com/linux_firmware/qa" ) );
    EXPECT_CALL( m_feedsUpdater, getFeedsSnapshot() ).WillOnce( Return( 25 ) );
    UpdateOperation operation = UpdateOperation::fromJson( R"({
        "operation": "sysupgrade",
        "type": "firmware"
})"_json );

    std::string expectedPlan = R"({
    "actions": [
        {
            "operation": "sysupgrade",
            "snapshot": 25,
            "source": "http://dl.mios.com/linux_firmware/qa",
            "type": "firmware",
            "version": "2.0.1.24"
        },
        {
            "id": "cameras",
            "operation": "install",
            "snapshot": 25,
            "source": "http://dl.mios.com/linux_firmware/qa",
            "type": "addon",
            "version": "1.0.24"
        }
    ]
})";

    auto planner = createUpdatePlanner( "2.0.1.24" );
    auto plan = planner.createUpdatePlan( operation );
    EXPECT_EQ( expectedPlan, plan->toJson().dump( 4 ) );
}

TEST_F( UpdatePlannerTestSuite, testUpgradeFirmwareExplicitlySpecifiedSourceUpgrade /*1.8.1*/ ) {
    EXPECT_CALL( m_packagesManager, getInstalledPackages() )
        .WillOnce( Return( Opkg::IPackagesManager::PackageSummaries( { { "hub-platform-system", "1.0.76" },
                                                                       { "hub-platform-firmware", "2.0.1.24" },
                                                                       { "firmware-addons-cameras", "1.0.24" } } ) ) );

    EXPECT_CALL( m_packagesManager, getAvailablePackages() )
        .WillOnce( Return( Opkg::IPackagesManager::PackageSummaries( { { "hub-platform-system", "1.0.76" },
                                                                       { "hub-platform-firmware", "2.0.1.24" },
                                                                       { "firmware-addons-cameras", "1.0.24" } } ) ) );

    EXPECT_CALL( m_feedsUpdater, getFeedsSource() ).WillOnce( Return( "http://dl.mios.com/linux_firmware/dev" ) );
    EXPECT_CALL( m_feedsUpdater, getFeedsSnapshot() ).WillOnce( Return( 24 ) );
    UpdateOperation operation = UpdateOperation::fromJson( R"({
        "operation": "upgrade",
        "type": "firmware",
        "source": "http://dl.mios.com/linux_firmware/dev"
})"_json );

    std::string expectedPlan = R"({
    "actions": [
        {
            "operation": "sysupgrade",
            "snapshot": 24,
            "source": "http://dl.mios.com/linux_firmware/dev",
            "type": "firmware",
            "version": "2.0.1.24"
        },
        {
            "id": "cameras",
            "operation": "install",
            "snapshot": 24,
            "source": "http://dl.mios.com/linux_firmware/dev",
            "type": "addon",
            "version": "1.0.24"
        }
    ]
})";

    auto planner = createUpdatePlanner( "2.0.1.24" );
    auto plan = planner.createUpdatePlan( operation );
    EXPECT_EQ( expectedPlan, plan->toJson().dump( 4 ) );
}

TEST_F( UpdatePlannerTestSuite, testUpgradeFirmwareExplicitlySpecifiedSourceSysupgrade /*1.8.2*/ ) {
    EXPECT_CALL( m_packagesManager, getInstalledPackages() )
        .WillOnce( Return( Opkg::IPackagesManager::PackageSummaries( { { "hub-platform-system", "1.0.76" },
                                                                       { "hub-platform-firmware", "2.0.1.24" },
                                                                       { "firmware-addons-cameras", "1.0.24" } } ) ) );

    EXPECT_CALL( m_packagesManager, getAvailablePackages() )
        .WillOnce( Return( Opkg::IPackagesManager::PackageSummaries( { { "hub-platform-system", "1.0.76" },
                                                                       { "hub-platform-firmware", "2.0.1.25" },
                                                                       { "firmware-addons-cameras", "1.0.24" } } ) ) );

    EXPECT_CALL( m_feedsUpdater, getFeedsSource() ).WillOnce( Return( "http://dl.mios.com/linux_firmware/dev" ) );
    EXPECT_CALL( m_feedsUpdater, getFeedsSnapshot() ).WillOnce( Return( 24 ) );
    UpdateOperation operation = UpdateOperation::fromJson( R"({
        "operation": "sysupgrade",
        "type": "firmware",
        "source": "http://dl.mios.com/linux_firmware/dev"
})"_json );

    std::string expectedPlan = R"({
    "actions": [
        {
            "operation": "sysupgrade",
            "snapshot": 24,
            "source": "http://dl.mios.com/linux_firmware/dev",
            "type": "firmware",
            "version": "2.0.1.25"
        },
        {
            "id": "cameras",
            "operation": "install",
            "snapshot": 24,
            "source": "http://dl.mios.com/linux_firmware/dev",
            "type": "addon",
            "version": "1.0.24"
        }
    ]
})";

    auto planner = createUpdatePlanner( "2.0.1.24" );
    auto plan = planner.createUpdatePlan( operation );
    EXPECT_EQ( expectedPlan, plan->toJson().dump( 4 ) );
}

TEST_F( UpdatePlannerTestSuite, testUpgradeFirmwareSysupgradeForNoReasonMeansUpgrade /*1.9*/ ) {
    EXPECT_CALL( m_packagesManager, getInstalledPackages() )
        .WillOnce( Return( Opkg::IPackagesManager::PackageSummaries( { { "hub-platform-system", "1.0.76" },
                                                                       { "hub-platform-firmware", "2.0.1.24" },
                                                                       { "firmware-addons-cameras", "1.0.24" } } ) ) );

    EXPECT_CALL( m_packagesManager, getAvailablePackages() )
        .WillOnce( Return( Opkg::IPackagesManager::PackageSummaries( { { "hub-platform-system", "1.0.76" },
                                                                       { "hub-platform-firmware", "2.0.1.25" },
                                                                       { "firmware-addons-cameras", "1.0.25" } } ) ) );

    EXPECT_CALL( m_feedsUpdater, getFeedsSource() ).WillOnce( Return( "http://dl.mios.com/linux_firmware/qa" ) );
    EXPECT_CALL( m_feedsUpdater, getFeedsSnapshot() ).WillOnce( Return( 25 ) );
    UpdateOperation operation = UpdateOperation::fromJson( R"({
        "operation": "sysupgrade",
        "type": "firmware"
})"_json );

    std::string expectedPlan = R"({
    "actions": [
        {
            "operation": "sysupgrade",
            "snapshot": 25,
            "source": "http://dl.mios.com/linux_firmware/qa",
            "type": "firmware",
            "version": "2.0.1.25"
        },
        {
            "id": "cameras",
            "operation": "install",
            "snapshot": 25,
            "source": "http://dl.mios.com/linux_firmware/qa",
            "type": "addon",
            "version": "1.0.25"
        }
    ]
})";

    auto planner = createUpdatePlanner( "2.0.1.24" );
    auto plan = planner.createUpdatePlan( operation );
    EXPECT_EQ( expectedPlan, plan->toJson().dump( 4 ) );
}

TEST_F( UpdatePlannerTestSuite, testUpgradeFirmwareSysupgradeSameFirmwareSameExtensions /*1.10*/ ) {
    EXPECT_CALL( m_packagesManager, getInstalledPackages() )
        .WillOnce( Return( Opkg::IPackagesManager::PackageSummaries( { { "hub-platform-system", "1.0.76" },
                                                                       { "hub-platform-firmware", "2.0.1.24" },
                                                                       { "firmware-addons-cameras", "1.0.24" } } ) ) );

    EXPECT_CALL( m_packagesManager, getAvailablePackages() )
        .WillOnce( Return( Opkg::IPackagesManager::PackageSummaries( { { "hub-platform-system", "1.0.76" },
                                                                       { "hub-platform-firmware", "2.0.1.24" },
                                                                       { "firmware-addons-cameras", "1.0.24" } } ) ) );

    EXPECT_CALL( m_feedsUpdater, getFeedsSource() ).WillOnce( Return( "http://dl.mios.com/linux_firmware/qa" ) );
    EXPECT_CALL( m_feedsUpdater, getFeedsSnapshot() ).WillOnce( Return( 24 ) );
    UpdateOperation operation = UpdateOperation::fromJson( R"({
        "operation": "sysupgrade",
        "type": "firmware"
})"_json );

    std::string expectedPlan = R"({
    "actions": []
})";

    auto planner = createUpdatePlanner( "2.0.1.24" );
    auto plan = planner.createUpdatePlan( operation );
    EXPECT_EQ( expectedPlan, plan->toJson().dump( 4 ) );
}

TEST_F( UpdatePlannerTestSuite, testUpgradeExtensionSameVersion /*2.1*/ ) {
    EXPECT_CALL( m_packagesManager, getInstalledPackages() )
        .WillOnce( Return( Opkg::IPackagesManager::PackageSummaries( { { "hub-platform-system", "1.0.76" },
                                                                       { "hub-platform-firmware", "2.0.1.24" },
                                                                       { "firmware-addons-cameras", "1.0.24" },
                                                                       { "firmware-plugins-zwave", "1.0.24" } } ) ) );

    EXPECT_CALL( m_packagesManager, getAvailablePackages() )
        .WillOnce( Return( Opkg::IPackagesManager::PackageSummaries( { { "hub-platform-system", "1.0.76" },
                                                                       { "hub-platform-firmware", "2.0.1.24" },
                                                                       { "firmware-addons-cameras", "1.0.24" },
                                                                       { "firmware-plugins-zwave", "1.0.25" } } ) ) );

    EXPECT_CALL( m_feedsUpdater, getFeedsSource() ).WillOnce( Return( "http://dl.mios.com/linux_firmware/qa" ) );
    EXPECT_CALL( m_feedsUpdater, getFeedsSnapshot() ).WillOnce( Return( 25 ) );
    UpdateOperation operation = UpdateOperation::fromJson( R"({
        "id": "cameras",
        "operation": "upgrade",
        "type": "addon"
})"_json );

    std::string expectedPlan = R"({
    "actions": [
        {
            "id": "zwave",
            "operation": "upgrade",
            "snapshot": 25,
            "source": "http://dl.mios.com/linux_firmware/qa",
            "type": "plugin",
            "version": "1.0.25"
        }
    ]
})";

    auto planner = createUpdatePlanner( "2.0.1.24" );
    auto plan = planner.createUpdatePlan( operation );
    EXPECT_EQ( expectedPlan, plan->toJson().dump( 4 ) );
}

TEST_F( UpdatePlannerTestSuite, testUpgradeExtensionNewVersion /*2.2*/ ) {
    EXPECT_CALL( m_packagesManager, getInstalledPackages() )
        .WillOnce( Return( Opkg::IPackagesManager::PackageSummaries( { { "hub-platform-system", "1.0.76" },
                                                                       { "hub-platform-firmware", "2.0.1.24" },
                                                                       { "firmware-addons-cameras", "1.0.24" },
                                                                       { "firmware-plugins-zwave", "1.0.24" } } ) ) );

    EXPECT_CALL( m_packagesManager, getAvailablePackages() )
        .WillOnce( Return( Opkg::IPackagesManager::PackageSummaries( { { "hub-platform-system", "1.0.76" },
                                                                       { "hub-platform-firmware", "2.0.1.24" },
                                                                       { "firmware-addons-cameras", "1.0.25" },
                                                                       { "firmware-plugins-zwave", "1.0.25" } } ) ) );

    EXPECT_CALL( m_feedsUpdater, getFeedsSource() ).WillOnce( Return( "http://dl.mios.com/linux_firmware/qa" ) );
    EXPECT_CALL( m_feedsUpdater, getFeedsSnapshot() ).WillOnce( Return( 25 ) );
    UpdateOperation operation = UpdateOperation::fromJson( R"({
        "id": "cameras",
        "operation": "upgrade",
        "type": "addon"
})"_json );

    std::string expectedPlan = R"({
    "actions": [
        {
            "id": "cameras",
            "operation": "upgrade",
            "snapshot": 25,
            "source": "http://dl.mios.com/linux_firmware/qa",
            "type": "addon",
            "version": "1.0.25"
        },
        {
            "id": "zwave",
            "operation": "upgrade",
            "snapshot": 25,
            "source": "http://dl.mios.com/linux_firmware/qa",
            "type": "plugin",
            "version": "1.0.25"
        }
    ]
})";

    auto planner = createUpdatePlanner( "2.0.1.24" );
    auto plan = planner.createUpdatePlan( operation );
    EXPECT_EQ( expectedPlan, plan->toJson().dump( 4 ) );
}

TEST_F( UpdatePlannerTestSuite, testUpgradeExtensionOldVersion /*2.3*/ ) {
    EXPECT_CALL( m_packagesManager, getInstalledPackages() )
        .WillOnce( Return( Opkg::IPackagesManager::PackageSummaries( { { "hub-platform-system", "1.0.76" },
                                                                       { "hub-platform-firmware", "2.0.1.24" },
                                                                       { "firmware-addons-cameras", "1.0.24" },
                                                                       { "firmware-plugins-zwave", "1.0.24" } } ) ) );

    EXPECT_CALL( m_packagesManager, getAvailablePackages() )
        .WillOnce( Return( Opkg::IPackagesManager::PackageSummaries( { { "hub-platform-system", "1.0.76" },
                                                                       { "hub-platform-firmware", "2.0.1.24" },
                                                                       { "firmware-addons-cameras", "1.0.23" },
                                                                       { "firmware-plugins-zwave", "1.0.25" } } ) ) );

    EXPECT_CALL( m_feedsUpdater, getFeedsSource() ).WillOnce( Return( "http://dl.mios.com/linux_firmware/qa" ) );
    EXPECT_CALL( m_feedsUpdater, getFeedsSnapshot() ).WillOnce( Return( 25 ) );
    UpdateOperation operation = UpdateOperation::fromJson( R"({
        "id": "cameras",
        "operation": "upgrade",
        "type": "addon"
})"_json );

    std::string expectedPlan = R"({
    "actions": [
        {
            "operation": "sysupgrade",
            "snapshot": 25,
            "source": "http://dl.mios.com/linux_firmware/qa",
            "type": "firmware",
            "version": "2.0.1.24"
        },
        {
            "id": "cameras",
            "operation": "install",
            "snapshot": 25,
            "source": "http://dl.mios.com/linux_firmware/qa",
            "type": "addon",
            "version": "1.0.23"
        },
        {
            "id": "zwave",
            "operation": "install",
            "snapshot": 25,
            "source": "http://dl.mios.com/linux_firmware/qa",
            "type": "plugin",
            "version": "1.0.25"
        }
    ]
})";

    auto planner = createUpdatePlanner( "2.0.1.24" );
    auto plan = planner.createUpdatePlan( operation );
    EXPECT_EQ( expectedPlan, plan->toJson().dump( 4 ) );
}

TEST_F( UpdatePlannerTestSuite, testInstallExtensionSameSnapshot /*3.1*/ ) {
    EXPECT_CALL( m_packagesManager, getInstalledPackages() )
        .WillOnce( Return( Opkg::IPackagesManager::PackageSummaries( { { "hub-platform-system", "1.0.76" },
                                                                       { "hub-platform-firmware", "2.0.1.24" },
                                                                       { "firmware-plugins-zwave", "1.0.24" } } ) ) );

    EXPECT_CALL( m_packagesManager, getAvailablePackages() )
        .WillOnce( Return( Opkg::IPackagesManager::PackageSummaries( { { "hub-platform-system", "1.0.76" },
                                                                       { "hub-platform-firmware", "2.0.1.24" },
                                                                       { "firmware-addons-cameras", "1.0.24" },
                                                                       { "firmware-plugins-zwave", "1.0.24" } } ) ) );

    EXPECT_CALL( m_feedsUpdater, getFeedsSource() ).WillOnce( Return( "http://dl.mios.com/linux_firmware/qa" ) );
    EXPECT_CALL( m_feedsUpdater, getFeedsSnapshot() ).WillOnce( Return( 24 ) );
    UpdateOperation operation = UpdateOperation::fromJson( R"({
        "id": "cameras",
        "operation": "install",
        "type": "addon",
        "version": "1.0.24"
})"_json );

    std::string expectedPlan = R"({
    "actions": [
        {
            "id": "cameras",
            "operation": "install",
            "snapshot": 24,
            "source": "http://dl.mios.com/linux_firmware/qa",
            "type": "addon",
            "version": "1.0.24"
        }
    ]
})";

    auto planner = createUpdatePlanner( "2.0.1.24" );
    auto plan = planner.createUpdatePlan( operation );
    EXPECT_EQ( expectedPlan, plan->toJson().dump( 4 ) );
}

TEST_F( UpdatePlannerTestSuite, testInstallExtensionNewSnapshot /*3.2*/ ) {
    EXPECT_CALL( m_packagesManager, getInstalledPackages() )
        .WillOnce( Return( Opkg::IPackagesManager::PackageSummaries( { { "hub-platform-system", "1.0.76" },
                                                                       { "hub-platform-firmware", "2.0.1.24" },
                                                                       { "firmware-plugins-zwave", "1.0.24" } } ) ) );

    EXPECT_CALL( m_packagesManager, getAvailablePackages() )
        .WillOnce( Return( Opkg::IPackagesManager::PackageSummaries( { { "hub-platform-system", "1.0.76" },
                                                                       { "hub-platform-firmware", "2.0.1.24" },
                                                                       { "firmware-addons-cameras", "1.0.25" },
                                                                       { "firmware-plugins-zwave", "1.0.25" } } ) ) );

    EXPECT_CALL( m_feedsUpdater, getFeedsSource() ).WillOnce( Return( "http://dl.mios.com/linux_firmware/qa" ) );
    EXPECT_CALL( m_feedsUpdater, getFeedsSnapshot() ).WillOnce( Return( 25 ) );
    UpdateOperation operation = UpdateOperation::fromJson( R"({
        "id": "cameras",
        "operation": "install",
        "type": "addon",
        "version": "1.0.25"
})"_json );

    std::string expectedPlan = R"({
    "actions": [
        {
            "id": "zwave",
            "operation": "upgrade",
            "snapshot": 25,
            "source": "http://dl.mios.com/linux_firmware/qa",
            "type": "plugin",
            "version": "1.0.25"
        },
        {
            "id": "cameras",
            "operation": "install",
            "snapshot": 25,
            "source": "http://dl.mios.com/linux_firmware/qa",
            "type": "addon",
            "version": "1.0.25"
        }
    ]
})";

    auto planner = createUpdatePlanner( "2.0.1.24" );
    auto plan = planner.createUpdatePlan( operation );
    EXPECT_EQ( expectedPlan, plan->toJson().dump( 4 ) );
}

TEST_F( UpdatePlannerTestSuite, testInstallExtensionOldSnapshot /*3.3*/ ) {
    EXPECT_CALL( m_packagesManager, getInstalledPackages() )
        .WillOnce( Return( Opkg::IPackagesManager::PackageSummaries( { { "hub-platform-system", "1.0.76" },
                                                                       { "hub-platform-firmware", "2.0.1.24" },
                                                                       { "firmware-plugins-zwave", "1.0.24" } } ) ) );

    EXPECT_CALL( m_packagesManager, getAvailablePackages() )
        .WillOnce( Return( Opkg::IPackagesManager::PackageSummaries( { { "hub-platform-system", "1.0.76" },
                                                                       { "hub-platform-firmware", "2.0.1.23" },
                                                                       { "firmware-addons-cameras", "1.0.23" },
                                                                       { "firmware-plugins-zwave", "1.0.23" } } ) ) );

    EXPECT_CALL( m_feedsUpdater, getFeedsSource() ).WillOnce( Return( "http://dl.mios.com/linux_firmware/qa" ) );
    EXPECT_CALL( m_feedsUpdater, getFeedsSnapshot() ).WillOnce( Return( 25 ) );
    UpdateOperation operation = UpdateOperation::fromJson( R"({
        "id": "cameras",
        "operation": "install",
        "type": "addon",
        "version": "1.0.23"
})"_json );

    std::string expectedPlan = R"({
    "actions": [
        {
            "operation": "sysupgrade",
            "snapshot": 25,
            "source": "http://dl.mios.com/linux_firmware/qa",
            "type": "firmware",
            "version": "2.0.1.23"
        },
        {
            "id": "zwave",
            "operation": "install",
            "snapshot": 25,
            "source": "http://dl.mios.com/linux_firmware/qa",
            "type": "plugin",
            "version": "1.0.23"
        },
        {
            "id": "cameras",
            "operation": "install",
            "snapshot": 25,
            "source": "http://dl.mios.com/linux_firmware/qa",
            "type": "addon",
            "version": "1.0.23"
        }
    ]
})";

    auto planner = createUpdatePlanner( "2.0.1.24" );
    auto plan = planner.createUpdatePlan( operation );
    EXPECT_EQ( expectedPlan, plan->toJson().dump( 4 ) );
}

TEST_F( UpdatePlannerTestSuite, testUninstallExtension /*4*/ ) {
    EXPECT_CALL( m_packagesManager, getDependantPackages( "firmware-addons-lua" ) )
        .WillOnce( Return( Opkg::IPackagesManager::PackageSummaries( { { "hub-platform-system", "1.0.76" },
                                                                       { "firmware-plugins-cameras", "1.0.24" },
                                                                       { "firmware-plugins-zwave", "1.0.24" } } ) ) );

    EXPECT_CALL( m_feedsUpdater, getFeedsSource() ).WillOnce( Return( "http://dl.mios.com/linux_firmware/qa" ) );
    EXPECT_CALL( m_feedsUpdater, getFeedsSnapshot() ).WillOnce( Return( 24 ) );
    UpdateOperation operation = UpdateOperation::fromJson( R"({
        "id": "lua",
        "operation": "uninstall",
        "type": "addon",
        "version": "1.0.24"
})"_json );

    std::string expectedPlan = R"({
    "actions": [
        {
            "id": "lua",
            "operation": "uninstall",
            "snapshot": 24,
            "source": "http://dl.mios.com/linux_firmware/qa",
            "type": "addon",
            "version": "1.0.24"
        },
        {
            "id": "cameras",
            "operation": "uninstall",
            "snapshot": 24,
            "source": "http://dl.mios.com/linux_firmware/qa",
            "type": "plugin",
            "version": "1.0.24"
        },
        {
            "id": "zwave",
            "operation": "uninstall",
            "snapshot": 24,
            "source": "http://dl.mios.com/linux_firmware/qa",
            "type": "plugin",
            "version": "1.0.24"
        }
    ]
})";

    auto planner = createUpdatePlanner( "2.0.1.24" );
    auto plan = planner.createUpdatePlan( operation );
    EXPECT_EQ( expectedPlan, plan->toJson().dump( 4 ) );
}

TEST_F( UpdatePlannerTestSuite, testPackageMissingAtNewSnapshot /*5.1*/ ) {
    EXPECT_CALL( m_packagesManager, getInstalledPackages() )
        .WillOnce( Return( Opkg::IPackagesManager::PackageSummaries( { { "hub-platform-system", "1.0.76" },
                                                                       { "hub-platform-firmware", "2.0.1.24" },
                                                                       { "firmware-addons-cameras", "1.0.24" },
                                                                       { "firmware-plugins-zwave", "1.0.24" } } ) ) );

    EXPECT_CALL( m_packagesManager, getAvailablePackages() )
        .WillOnce( Return( Opkg::IPackagesManager::PackageSummaries(
            { { "hub-platform-system", "1.0.76" }, { "hub-platform-firmware", "2.0.1.25" } } ) ) );

    EXPECT_CALL( m_feedsUpdater, getFeedsSource() ).WillOnce( Return( "http://dl.mios.com/linux_firmware/qa" ) );
    EXPECT_CALL( m_feedsUpdater, getFeedsSnapshot() ).WillOnce( Return( 25 ) );
    UpdateOperation operation = UpdateOperation::fromJson( R"({
        "operation": "upgrade",
        "type": "firmware",
        "version": "2.0.1.25"
})"_json );

    std::string expectedPlan = R"({
    "actions": [
        {
            "operation": "upgrade",
            "snapshot": 25,
            "source": "http://dl.mios.com/linux_firmware/qa",
            "type": "firmware",
            "version": "2.0.1.25"
        },
        {
            "id": "cameras",
            "operation": "uninstall",
            "snapshot": 25,
            "source": "http://dl.mios.com/linux_firmware/qa",
            "type": "addon",
            "version": "1.0.24"
        },
        {
            "id": "zwave",
            "operation": "uninstall",
            "snapshot": 25,
            "source": "http://dl.mios.com/linux_firmware/qa",
            "type": "plugin",
            "version": "1.0.24"
        }
    ]
})";

    auto planner = createUpdatePlanner( "2.0.1.24" );
    auto plan = planner.createUpdatePlan( operation );
    EXPECT_EQ( expectedPlan, plan->toJson().dump( 4 ) );
}

// Check the custom_plugins update command validator doesn't touch ordinal update plan actions
TEST_F( UpdatePlannerTestSuite, customPluginOperationsValidateOrdinalPlan ) /* 5.2 */ {
    std::string expectedPlan = R"({
    "actions": [
        {
            "operation": "sysupgrade",
            "snapshot": 25,
            "source": "http://dl.mios.com/linux_firmware/qa",
            "type": "firmware",
            "version": "2.0.1.23"
        },
        {
            "id": "cameras",
            "operation": "install",
            "snapshot": 25,
            "source": "http://dl.mios.com/linux_firmware/qa",
            "type": "addon",
            "version": "1.0.24"
        },
        {
            "id": "luad",
            "operation": "uninstall",
            "snapshot": 25,
            "source": "http://dl.mios.com/linux_firmware/qa",
            "type": "addon",
            "version": "1.0.24"
        },
        {
            "id": "luad",
            "operation": "upgrade",
            "snapshot": 25,
            "source": "http://dl.mios.com/linux_firmware/qa",
            "type": "addon",
            "version": "1.0.24"
        }
    ]
})";

    std::string errorMessage, errorField;
    UpdatePlan::validateCustomPluginActions( expectedPlan, errorMessage, errorField );
}

TEST_F( UpdatePlannerTestSuite, customPluginOperationsValidateCustomPlugin ) /* 5.3 */ {
    std::string expectedPlan = R"({
    "actions": [
        {
            "id": "marketplace.plugin_name",
            "operation": "install",
            "source": "http://dl.mios.com/linux_firmware/plugin_storage/marketplace.plugin_name.tgz",
            "type": "custom_plugin",
            "version": "1.0.24",
            "archive_hash_md5": "deadBeef"
        },
        {
            "id": "marketplace.plugin_name",
            "operation": "upgrade",
            "source": "http://dl.mios.com/linux_firmware/plugin_storage/marketplace.plugin_name.tgz",
            "type": "custom_plugin",
            "version": "1.0.24",
            "archive_hash_md5": "deadBeef"
        },
        {
            "id": "marketplace.plugin_name",
            "operation": "uninstall",
            "type": "custom_plugin"
        }
    ]
})";

    std::string errorMessage, errorField;
    UpdatePlan::validateCustomPluginActions( expectedPlan, errorMessage, errorField );
}

TEST( UpdateOperationTestSuite, checkSerializationDeserialization /*6.1*/ ) {
    const std::string updateOperationString = R"(
	{
	   "operation": "upgrade",
	   "source": "http://dl.mios.com/linux_firmware/qa",
	   "snapshot": 42,
	   "type": "plugin",
	   "id": "zwave",
	   "version": "1.3.42"	}
	)";

    const auto updateOperationJson = Json::parse( updateOperationString );
    const auto updateOperation = UpdateOperation::fromJson( updateOperationJson );

    EXPECT_EQ( updateOperation.toJson(), updateOperationJson );
}

TEST( UpdateOperationTestSuite, checkInvalidOperationTypeFail /*6.2*/ ) {
    const std::string updateOperationString = R"(
	{
	   "operation": "invalidOperationType",
	   "source": "http://dl.mios.com/linux_firmware/qa",
	   "snapshot": 42,
	   "type": "plugin",
	   "id": "zwave",
	   "version": "1.3.42"	}
	)";

    const auto updateOperation = UpdateOperation::fromJson( Json::parse( updateOperationString ) );
    EXPECT_FALSE( updateOperation.isValid() );
}

TEST( UpdateOperationTestSuite, checkInvalidTypeFail /*6.3*/ ) {
    const std::string updateOperationString = R"(
	{
	   "operation": "upgrade",
	   "source": "http://dl.mios.com/linux_firmware/qa",
	   "snapshot": 42,
	   "type": "invalidType",
	   "id": "zwave",
	   "version": "1.3.42"	}
	)";

    const auto updateOperation = UpdateOperation::fromJson( Json::parse( updateOperationString ) );
    EXPECT_FALSE( updateOperation.isValid() );
}
