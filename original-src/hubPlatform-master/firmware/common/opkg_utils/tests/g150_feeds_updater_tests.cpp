#include "opkg_utils/feeds/g150_feeds_updater.h"

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
DONE        1.4 Without source and snapshot, all feeds
DONE            1.4.1 Without source and snapshot, common feeds
DONE            1.4.2 Without source and snapshot, custom feeds
DONE        1.5 With negative snapshot
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

class G150FeedsUpdaterTestSuite : public Test {
public:
    G150FeedsUpdaterTestSuite() {
        // Do not print errors and warnings on erroneous test cases
        // TODO: implement fake log and check error messages too.
        Log::setLocalLogLevel( Log::FATAL );

        filesystem::createFolder( m_tmpDir.getPath() + "/etc/opkg" );
        setFeedsContent( "" );
    }

    std::unique_ptr< G150FeedsUpdater > createG150FeedsUpdater(
        IFeedsUpdater::Feeds feedsType = IFeedsUpdater::Feeds::Common ) {
        return std::make_unique< G150FeedsUpdater >( createFeedsConfiguration(), feedsType );
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

TEST_F( G150FeedsUpdaterTestSuite, testUpdateWithSourceAndSnapshot /*1.1*/ ) {
    auto updater = createG150FeedsUpdater();

    EXPECT_TRUE( updater->update( "http://dl.mios.com/linux_firmware/qa", 42 ) );
    EXPECT_EQ( getFeedsContent(),
               "src/gz system http://dl.mios.com/linux_firmware/qa/bundle/g150/v2/snapshots/42/system\n"
               "src/gz firmware http://dl.mios.com/linux_firmware/qa/bundle/g150/v2/snapshots/42/firmware\n"
               "src/gz plugins http://dl.mios.com/linux_firmware/qa/bundle/g150/v2/snapshots/42/plugins\n" );
}

TEST_F( G150FeedsUpdaterTestSuite, testUpdateWithSourceWithoutSnapshot /*1.2*/ ) {
    setFeedsContent( "src/gz system http://dl.mios.com/linux_firmware/dev/bundle/g150/v2/snapshots/42/system\n" );

    auto updater = createG150FeedsUpdater();

    EXPECT_TRUE( updater->update( "http://dl.mios.com/linux_firmware/qa", 0 ) );
    EXPECT_EQ( getFeedsContent(),
               "src/gz system http://dl.mios.com/linux_firmware/qa/bundle/g150/v2/snapshots/42/system\n"
               "src/gz firmware http://dl.mios.com/linux_firmware/qa/bundle/g150/v2/snapshots/42/firmware\n"
               "src/gz plugins http://dl.mios.com/linux_firmware/qa/bundle/g150/v2/snapshots/42/plugins\n" );
}

TEST_F( G150FeedsUpdaterTestSuite, testUpdateWithSnapshotWithoutSourceEmptyFeeds /*1.3.1*/ ) {
    auto updater = createG150FeedsUpdater();

    EXPECT_FALSE( updater->update( "", 42 ) );
    EXPECT_EQ( getFeedsContent(), "" );
}

TEST_F( G150FeedsUpdaterTestSuite, testUpdateWithSnapshotWithoutSourceCorrectFeeds /*1.3.2*/ ) {
    setFeedsContent( "src/gz system http://dl.mios.com/linux_firmware/qa/bundle/g150/v2/latest/system\n" );

    auto updater = createG150FeedsUpdater();

    EXPECT_TRUE( updater->update( "", 42 ) );
    EXPECT_EQ( getFeedsContent(),
               "src/gz system http://dl.mios.com/linux_firmware/qa/bundle/g150/v2/snapshots/42/system\n"
               "src/gz firmware http://dl.mios.com/linux_firmware/qa/bundle/g150/v2/snapshots/42/firmware\n"
               "src/gz plugins http://dl.mios.com/linux_firmware/qa/bundle/g150/v2/snapshots/42/plugins\n" );
}

TEST_F( G150FeedsUpdaterTestSuite, testUpdateWithoutSourceAndSnapshot /*1.4*/ ) {
    setFeedsContent( "src/gz system http://dl.mios.com/linux_firmware/qa/bundle/g150/v2/snapshots/42/system\n" );

    auto updater = createG150FeedsUpdater( IFeedsUpdater::Feeds::All );

    EXPECT_TRUE( updater->update( "", 0 ) );
    EXPECT_EQ(
        getFeedsContent(),
        "src/gz system http://dl.mios.com/linux_firmware/qa/bundle/g150/v2/snapshots/42/system\n"
        "src/gz firmware http://dl.mios.com/linux_firmware/qa/bundle/g150/v2/snapshots/42/firmware\n"
        "src/gz plugins http://dl.mios.com/linux_firmware/qa/bundle/g150/v2/snapshots/42/plugins\n"
        "src/gz custom_packages http://dl.mios.com/linux_firmware/qa/bundle/g150/v2/snapshots/42/custom_packages\n" );
}

TEST_F( G150FeedsUpdaterTestSuite, testUpdateWithoutSourceAndSnapshotCommon /*1.4.1*/ ) {
    setFeedsContent( "src/gz system http://dl.mios.com/linux_firmware/qa/bundle/g150/v2/snapshots/42/system\n" );

    auto updater = createG150FeedsUpdater( IFeedsUpdater::Feeds::Common );

    EXPECT_TRUE( updater->update( "", 0 ) );
    EXPECT_EQ( getFeedsContent(),
               "src/gz system http://dl.mios.com/linux_firmware/qa/bundle/g150/v2/snapshots/42/system\n"
               "src/gz firmware http://dl.mios.com/linux_firmware/qa/bundle/g150/v2/snapshots/42/firmware\n"
               "src/gz plugins http://dl.mios.com/linux_firmware/qa/bundle/g150/v2/snapshots/42/plugins\n" );
}

TEST_F( G150FeedsUpdaterTestSuite, testUpdateWithoutSourceAndSnapshotCustomPackages /*1.4.2*/ ) {
    setFeedsContent( "src/gz system http://dl.mios.com/linux_firmware/qa/bundle/g150/v2/snapshots/42/system\n" );

    auto updater = createG150FeedsUpdater( IFeedsUpdater::Feeds::Custom );

    EXPECT_TRUE( updater->update( "", 0 ) );
    EXPECT_EQ(
        getFeedsContent(),
        "src/gz custom_packages http://dl.mios.com/linux_firmware/qa/bundle/g150/v2/snapshots/42/custom_packages\n" );
}

TEST_F( G150FeedsUpdaterTestSuite, testUpdateWithNegativeSnapshot /*1.5*/ ) {
    setFeedsContent( "src/gz system "
                     "http://dl.mios.com/linux_firmware/qa/bundle/g150/v2/snapshots/42/system\n" );

    auto updater = createG150FeedsUpdater();

    EXPECT_TRUE( updater->update( "", -1 ) );
    EXPECT_EQ( getFeedsContent(),
               "src/gz system http://dl.mios.com/linux_firmware/qa/bundle/g150/v2/latest/system\n"
               "src/gz firmware http://dl.mios.com/linux_firmware/qa/bundle/g150/v2/latest/firmware\n"
               "src/gz plugins http://dl.mios.com/linux_firmware/qa/bundle/g150/v2/latest/plugins\n" );
}

TEST_F( G150FeedsUpdaterTestSuite, testUpdateFromUrl /*1.6*/ ) {
    auto updater = createG150FeedsUpdater();
    updater->update( "http://dl.mios.com/linux_firmware/qa/bundle/g150/v2/snapshots/42/g150_sysupgrade.bin" );

    EXPECT_EQ( getFeedsContent(),
               "src/gz system http://dl.mios.com/linux_firmware/qa/bundle/g150/v2/snapshots/42/system\n"
               "src/gz firmware http://dl.mios.com/linux_firmware/qa/bundle/g150/v2/snapshots/42/firmware\n"
               "src/gz plugins http://dl.mios.com/linux_firmware/qa/bundle/g150/v2/snapshots/42/plugins\n" );
}

TEST_F( G150FeedsUpdaterTestSuite, testGenerateImageUrl /*2.1*/ ) {
    setFeedsContent( "src/gz system http://dl.mios.com/linux_firmware/qa/bundle/g150/v2/snapshots/42/system\n" );

    auto updater = createG150FeedsUpdater();

    EXPECT_EQ( updater->generateImageUrl(),
               "http://dl.mios.com/linux_firmware/qa/bundle/g150/v2/snapshots/42/g150_sysupgrade.bin" );
}

TEST_F( G150FeedsUpdaterTestSuite, testGetCurrentSource /*3.1*/ ) {
    setFeedsContent( "src/gz system http://dl.mios.com/linux_firmware/qa/bundle/g150/v2/snapshots/42/system\n" );

    auto updater = createG150FeedsUpdater();

    EXPECT_EQ( updater->getFeedsSource(), "http://dl.mios.com/linux_firmware/qa" );
}

TEST_F( G150FeedsUpdaterTestSuite, testGetCurrentUpdateVersion /*3.2*/ ) {
    setFeedsContent( "src/gz system http://dl.mios.com/linux_firmware/qa/bundle/g150/v2/snapshots/42/system\n" );

    auto updater = createG150FeedsUpdater();

    EXPECT_EQ( updater->getFeedsUpdateVersion(), "v2" );
}

TEST_F( G150FeedsUpdaterTestSuite, testGetCurrentSnapshotCorrect /*3.3*/ ) {
    setFeedsContent( "src/gz system http://dl.mios.com/linux_firmware/qa/bundle/g150/v2/snapshots/42/system\n" );

    auto updater = createG150FeedsUpdater();

    EXPECT_EQ( updater->getFeedsSnapshot(), 42 );
}

TEST_F( G150FeedsUpdaterTestSuite, testGetCurrentSnapshotLatest /*3.4*/ ) {
    setFeedsContent( "src/gz system http://dl.mios.com/linux_firmware/qa/bundle/g150/v2/snapshots/latest/system\n" );

    auto updater = createG150FeedsUpdater();

    EXPECT_EQ( updater->getFeedsSnapshot(), -1 );
}
