#include "opkg_utils/feeds/h2_feeds_updater.h"

#include "system_utils/filesystem.h"
#include "test_utils/temporary_folder_wrapper.h"

#include "log_utils/ezlo_log.h"
#include "json_utils/json_utils.h"
#include "common_utils/cpp14.h"

#include <gtest/gtest.h>

#include <fstream>

/*

TESTPLAN

DONE    1 Update
DONE        1.1 With source and snapshot
DONE        1.2 With source, without snapshot
DONE        1.3 With snapshot without source
DONE            1.3.1 Empty feeds
DONE            1.3.2 Correct feeds
DONE        1.4 Without source and snapshot
DONE        1.5 With nagative snapshot
DONE        1.6 From URL
DONE    2 Image URL
DONE        2.1 Generate image URL
DONE    3 Feeds info
DONE        3.1 Get feeds source
DONE        3.2 Get feeds update version
DONE        3.3 Get feeds snapshot (correct)
DONE        3.4 Get feeds snapshot (latest)

*/

using namespace ::testing;
using namespace HubPlatform;
using namespace HubPlatform::Opkg;

class H2FeedsUpdaterTestSuite : public Test {
public:
    H2FeedsUpdaterTestSuite() {
        // Do not print errors and warnings on erroneous test cases
        // TODO: implement fake log and check error messages too.
        Log::setLocalLogLevel( Log::DEBUG );

        filesystem::createFolder( m_tmpDir.getPath() + "/etc/opkg" );
        setFeedsContent( "" );
    }

    std::unique_ptr< H2FeedsUpdater > createH2FeedsUpdater(
        IFeedsUpdater::Feeds feedsType = IFeedsUpdater::Feeds::All ) {
        return std::make_unique< H2FeedsUpdater >( createFeedsConfiguration(), feedsType );
    }

    std::string getFeedsContent() {
        std::ifstream distfeeds( m_tmpDir.getPath() + "/etc/opkg/distfeeds.conf" );
        std::string content( ( std::istreambuf_iterator< char >( distfeeds ) ), std::istreambuf_iterator< char >() );
        return content;
    }

    void setFeedsContent( const std::string& content ) {
        std::ofstream distfeeds( m_tmpDir.getPath() + "/etc/opkg/distfeeds.conf" );
        distfeeds << content;
    }

private:
    std::unique_ptr< Opkg::FeedsConfiguration > createFeedsConfiguration() {
        return std::make_unique< Opkg::FeedsConfiguration >( m_tmpDir.getPath() + "/etc/opkg/distfeeds.conf" );
    }

    HubPlatform::Tests::TemporaryFolderWrapper m_tmpDir;
};

namespace Resources {
const std::string H2CommonFeeds_Qa42 =
    "src/gz ezlo_openwrt_core "
    "http://dl.mios.com/linux_firmware/qa/bundle/h2.1/v1/snapshots/42/system/targets/sunxi/cortexa7/packages\n"
    "src/gz ezlo_openwrt_base "
    "http://dl.mios.com/linux_firmware/qa/bundle/h2.1/v1/snapshots/42/system/packages/arm_cortex-a7_neon-vfpv4/"
    "base\n"
    "src/gz ezlo_openwrt_hub_platform_deps "
    "http://dl.mios.com/linux_firmware/qa/bundle/h2.1/v1/snapshots/42/system/packages/arm_cortex-a7_neon-vfpv4/"
    "hub_platform_deps\n"
    "src/gz ezlo_openwrt_packages "
    "http://dl.mios.com/linux_firmware/qa/bundle/h2.1/v1/snapshots/42/system/packages/arm_cortex-a7_neon-vfpv4/"
    "packages\n"
    "src/gz ezlo_openwrt_hub_platform "
    "http://dl.mios.com/linux_firmware/qa/bundle/h2.1/v1/snapshots/42/firmware\n"
    "src/gz ezlo_openwrt_hub_plugins "
    "http://dl.mios.com/linux_firmware/qa/bundle/h2.1/v1/snapshots/42/plugins\n";
const std::string H2CustomFeedsQa42 =
    "src/gz ezlo_openwrt_custom_packages "
    "http://dl.mios.com/linux_firmware/qa/bundle/h2.1/v1/snapshots/42/custom_packages\n";
}  // namespace Resources

TEST_F( H2FeedsUpdaterTestSuite, testUpdateWithSourceAndSnapshotForCommonFeeds /*1.1*/ ) {
    auto updater = createH2FeedsUpdater( IFeedsUpdater::Feeds::Common );

    EXPECT_TRUE( updater->update( "http://dl.mios.com/linux_firmware/qa", 42 ) );

    std::string expectedFeeds = Resources::H2CommonFeeds_Qa42;
    std::string fetchedFeeds = getFeedsContent();
    EXPECT_EQ( fetchedFeeds, expectedFeeds );
}

TEST_F( H2FeedsUpdaterTestSuite, testUpdateWithSourceAndSnapshotForCustomFeeds /*1.1.1*/ ) {
    auto updater = createH2FeedsUpdater( IFeedsUpdater::Feeds::Custom );

    EXPECT_TRUE( updater->update( "http://dl.mios.com/linux_firmware/qa", 42 ) );

    std::string expectedFeeds = Resources::H2CustomFeedsQa42;
    std::string fetchedFeeds = getFeedsContent();
    EXPECT_EQ( fetchedFeeds, expectedFeeds );
}

TEST_F( H2FeedsUpdaterTestSuite, testUpdateWithSourceAndSnapshotForAllFeeds /*1.1.2*/ ) {
    auto updater = createH2FeedsUpdater( IFeedsUpdater::Feeds::All );

    EXPECT_TRUE( updater->update( "http://dl.mios.com/linux_firmware/qa", 42 ) );

    std::string expectedFeeds = Resources::H2CommonFeeds_Qa42 + Resources::H2CustomFeedsQa42;
    std::string fetchedFeeds = getFeedsContent();
    EXPECT_EQ( fetchedFeeds, expectedFeeds );
}

TEST_F( H2FeedsUpdaterTestSuite, testUpdateWithSourceWithoutSnapshot /*1.2*/ ) {
    setFeedsContent( "src/gz openwrt_core "
                     "http://dl.mios.com/linux_firmware/dev/bundle/h2.1/v1/snapshots/42/system/targets/sunxi/"
                     "cortexa7/packages\n" );

    auto updater = createH2FeedsUpdater( IFeedsUpdater::Feeds::All );

    EXPECT_TRUE( updater->update( "http://dl.mios.com/linux_firmware/qa", 0 ) );

    std::string expectedFeeds = Resources::H2CommonFeeds_Qa42 + Resources::H2CustomFeedsQa42;

    EXPECT_EQ( getFeedsContent(), expectedFeeds );
}

TEST_F( H2FeedsUpdaterTestSuite, testUpdateWithSnapshotWithoutSourceEmptyFeeds /*1.3.1*/ ) {
    auto updater = createH2FeedsUpdater();

    EXPECT_FALSE( updater->update( "", 42 ) );
    EXPECT_EQ( getFeedsContent(), "" );
}

TEST_F( H2FeedsUpdaterTestSuite, testUpdateWithSnapshotWithoutSourceCorrectFeeds /*1.3.2*/ ) {
    setFeedsContent(
        "src/gz openwrt_core "
        "http://dl.mios.com/linux_firmware/qa/bundle/h2.1/v1/latest/system/targets/sunxi/cortexa7/packages\n" );

    auto updater = createH2FeedsUpdater( IFeedsUpdater::Feeds::Common );

    EXPECT_TRUE( updater->update( "", 42 ) );
    EXPECT_EQ( getFeedsContent(), Resources::H2CommonFeeds_Qa42 );
}

TEST_F( H2FeedsUpdaterTestSuite, testUpdateWithoutSourceAndSnapshot /*1.4*/ ) {
    setFeedsContent( "src/gz openwrt_core "
                     "http://dl.mios.com/linux_firmware/qa/bundle/h2.1/v1/snapshots/42/system/targets/sunxi/"
                     "cortexa7/packages\n" );

    auto updater = createH2FeedsUpdater();

    EXPECT_TRUE( updater->update( "", 0 ) );
    EXPECT_EQ( getFeedsContent(), Resources::H2CommonFeeds_Qa42 + Resources::H2CustomFeedsQa42 );
}

TEST_F( H2FeedsUpdaterTestSuite, testUpdateWithoutNegativeSnapshot /*1.5*/ ) {
    setFeedsContent( "src/gz openwrt_core "
                     "http://dl.mios.com/linux_firmware/qa/bundle/h2.1/v1/snapshots/42/system/targets/sunxi/"
                     "cortexa7/packages\n" );

    auto updater = createH2FeedsUpdater( IFeedsUpdater::Feeds::All );

    EXPECT_TRUE( updater->update( "", -1 ) );
    EXPECT_EQ(
        getFeedsContent(),
        "src/gz ezlo_openwrt_core "
        "http://dl.mios.com/linux_firmware/qa/bundle/h2.1/v1/latest/system/targets/sunxi/cortexa7/packages\n"
        "src/gz ezlo_openwrt_base "
        "http://dl.mios.com/linux_firmware/qa/bundle/h2.1/v1/latest/system/packages/arm_cortex-a7_neon-vfpv4/base\n"
        "src/gz ezlo_openwrt_hub_platform_deps "
        "http://dl.mios.com/linux_firmware/qa/bundle/h2.1/v1/latest/system/packages/arm_cortex-a7_neon-vfpv4/"
        "hub_platform_deps\n"
        "src/gz ezlo_openwrt_packages "
        "http://dl.mios.com/linux_firmware/qa/bundle/h2.1/v1/latest/system/packages/arm_cortex-a7_neon-vfpv4/"
        "packages\n"
        "src/gz ezlo_openwrt_hub_platform "
        "http://dl.mios.com/linux_firmware/qa/bundle/h2.1/v1/latest/firmware\n"
        "src/gz ezlo_openwrt_hub_plugins "
        "http://dl.mios.com/linux_firmware/qa/bundle/h2.1/v1/latest/plugins\n"
        "src/gz ezlo_openwrt_custom_packages "
        "http://dl.mios.com/linux_firmware/qa/bundle/h2.1/v1/latest/custom_packages\n" );
}

TEST_F( H2FeedsUpdaterTestSuite, testUpdateFromUrl /*1.6*/ ) {
    auto updater = createH2FeedsUpdater( IFeedsUpdater::Feeds::All );
    updater->update( "http://dl.mios.com/linux_firmware/qa/bundle/h2.1/v1/snapshots/42/"
                     "ezlo-openwrt-sunxi-cortexa7-sun8i-h2-plus-orangepi-zero-squashfs-sdcard.img.gz" );

    EXPECT_EQ( getFeedsContent(), Resources::H2CommonFeeds_Qa42 + Resources::H2CustomFeedsQa42 );
}

TEST_F( H2FeedsUpdaterTestSuite, testGenerateImageUrl /*2.1*/ ) {
    setFeedsContent( "src/gz openwrt_core "
                     "http://dl.mios.com/linux_firmware/qa/bundle/h2.1/v1/snapshots/42/system/targets/sunxi/"
                     "cortexa7/packages\n" );

    auto updater = createH2FeedsUpdater();

    EXPECT_EQ( updater->generateImageUrl(),
               "http://dl.mios.com/linux_firmware/qa/bundle/h2.1/v1/snapshots/42/"
               "ezlo-openwrt-sunxi-cortexa7-sun8i-h2-plus-orangepi-zero-squashfs-sdcard.img.gz" );
}

TEST_F( H2FeedsUpdaterTestSuite, testGetCurrentSource /*3.1*/ ) {
    setFeedsContent( "src/gz openwrt_core "
                     "http://dl.mios.com/linux_firmware/qa/bundle/h2.1/v1/snapshots/42/system/targets/sunxi/"
                     "cortexa7/packages\n" );

    auto updater = createH2FeedsUpdater();

    EXPECT_EQ( updater->getFeedsSource(), "http://dl.mios.com/linux_firmware/qa" );
}

TEST_F( H2FeedsUpdaterTestSuite, testGetCurrentUpdateVersion /*3.2*/ ) {
    setFeedsContent( "src/gz openwrt_core "
                     "http://dl.mios.com/linux_firmware/qa/bundle/h2.1/v1/snapshots/42/system/targets/sunxi/"
                     "cortexa7/packages\n" );

    auto updater = createH2FeedsUpdater();

    EXPECT_EQ( updater->getFeedsUpdateVersion(), "v1" );
}

TEST_F( H2FeedsUpdaterTestSuite, testGetCurrentSnapshotCorrect /*3.3*/ ) {
    setFeedsContent( "src/gz openwrt_core "
                     "http://dl.mios.com/linux_firmware/qa/bundle/h2.1/v1/snapshots/42/system/targets/sunxi/"
                     "cortexa7/packages\n" );

    auto updater = createH2FeedsUpdater();

    EXPECT_EQ( updater->getFeedsSnapshot(), 42 );
}

TEST_F( H2FeedsUpdaterTestSuite, testGetCurrentSnapshotLatest /*3.4*/ ) {
    setFeedsContent( "src/gz openwrt_core "
                     "http://dl.mios.com/linux_firmware/qa/bundle/h2.1/v1/snapshots/latest/system/targets/sunxi/"
                     "cortexa7/packages\n" );

    auto updater = createH2FeedsUpdater();

    EXPECT_EQ( updater->getFeedsSnapshot(), -1 );
}
