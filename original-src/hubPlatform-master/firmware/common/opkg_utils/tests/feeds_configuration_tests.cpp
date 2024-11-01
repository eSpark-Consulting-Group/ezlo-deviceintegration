#include "opkg_utils/feeds/feeds_configuration.h"

#include "opkg_utils/tests/resources/opkg_test_resources.h"

#include "system_utils/filesystem.h"
#include "test_utils/temporary_folder_wrapper.h"

#include "bus/messagesclasses.h"
#include "json_utils/json_utils.h"
#include "log_utils/ezlo_log.h"
#include "common_utils/cpp14.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

/*

TESTPLAN

DONE    1 Read feeds
DONE        1.1 Get default feeds
DONE        1.2 Few subsequent reads
DONE        1.3 Get unexisting feed
DONE    2 Modify feeds
DONE        2.1 Clear feeds
DONE        2.2 Remove feeds
DONE        2.3 Add feeds
DONE        2.4 Modify feeds
DONE    3 Transactions
DONE        3.1 Explicit revert
DONE        3.2 Implicit revert

*/

using namespace ::testing;
using namespace HubPlatform;
using namespace HubPlatform::Opkg;

class FeedsConfigurationTestSuite : public Test {
public:
    FeedsConfigurationTestSuite() {
        // Do not print errors and warnings on erroneous test cases
        // TODO: implement fake log and check error messages too.
        Log::setLocalLogLevel( Log::FATAL );

        filesystem::createFolder( m_tmpDir.getPath() + "/etc/opkg" );
        filesystem::copyFile( std::string( Resources::TestFilesPath ) + "/etc/opkg/distfeeds.conf",
                              m_tmpDir.getPath() + "/etc/opkg/distfeeds.conf" );
    }

    std::unique_ptr< FeedsConfiguration > createFeedsConfiguration() {
        return std::make_unique< FeedsConfiguration >( m_tmpDir.getPath() + "/etc/opkg/distfeeds.conf" );
    }

private:
    HubPlatform::Tests::TemporaryFolderWrapper m_tmpDir;
};

TEST_F( FeedsConfigurationTestSuite, testGetDefaultFeeds /*1.1*/ ) {
    auto feedsConfiguration = createFeedsConfiguration();

    EXPECT_EQ( feedsConfiguration->getFeedsCount(), 6 );
    EXPECT_EQ( Feed::toString( feedsConfiguration->getFeed( 0 ) ),
               "src/gz ezlo_openwrt_core "
               "http://dl.mios.com/linux_firmware/dev/bundle/h2.1/v1/latest/system/targets/sunxi/cortexa7/packages" );
    EXPECT_EQ( Feed::toString( feedsConfiguration->getFeed( 1 ) ),
               "src/gz ezlo_openwrt_base "
               "http://dl.mios.com/linux_firmware/dev/bundle/h2.1/v1/latest/system/packages/"
               "arm_cortex-a7_neon-vfpv4/base" );
    EXPECT_EQ( Feed::toString( feedsConfiguration->getFeed( 2 ) ),
               "src/gz ezlo_openwrt_hub_platform_deps "
               "http://dl.mios.com/linux_firmware/dev/bundle/h2.1/v1/latest/system/packages/"
               "arm_cortex-a7_neon-vfpv4/hub_platform_deps" );
    EXPECT_EQ( Feed::toString( feedsConfiguration->getFeed( 3 ) ),
               "src/gz ezlo_openwrt_packages "
               "http://dl.mios.com/linux_firmware/dev/bundle/h2.1/v1/latest/system/packages/"
               "arm_cortex-a7_neon-vfpv4/packages" );
    EXPECT_EQ( Feed::toString( feedsConfiguration->getFeed( 4 ) ),
               "src/gz ezlo_openwrt_hub_platform "
               "http://dl.mios.com/linux_firmware/dev/bundle/h2.1/v1/latest/firmware" );
    EXPECT_EQ( Feed::toString( feedsConfiguration->getFeed( 5 ) ),
               "src/gz ezlo_openwrt_hub_plugins http://dl.mios.com/linux_firmware/dev/bundle/h2.1/v1/latest/plugins" );
}

TEST_F( FeedsConfigurationTestSuite, testFewSubsequentReads /*1.2*/ ) {
    {
        auto feedsConfiguration = createFeedsConfiguration();
        EXPECT_EQ( feedsConfiguration->getFeedsCount(), 6 );
    }

    {
        auto feedsConfiguration = createFeedsConfiguration();
        EXPECT_EQ( feedsConfiguration->getFeedsCount(), 6 );
    }
}

TEST_F( FeedsConfigurationTestSuite, testGetUnexistingFeeds /*1.3*/ ) {
    auto feedsConfiguration = createFeedsConfiguration();

    EXPECT_EQ( feedsConfiguration->getFeedsCount(), 6 );

    EXPECT_FALSE( feedsConfiguration->getFeed( 42 ).isValid() );
    EXPECT_FALSE( feedsConfiguration->takeFeed( 42 ).isValid() );
}

TEST_F( FeedsConfigurationTestSuite, testClearFeeds /*2.1*/ ) {
    {
        auto feedsConfiguration = createFeedsConfiguration();
        feedsConfiguration->clearFeeds();
        feedsConfiguration->commit();
    }

    auto feedsConfiguration = createFeedsConfiguration();
    EXPECT_EQ( feedsConfiguration->getFeedsCount(), 0 );
}

TEST_F( FeedsConfigurationTestSuite, testRemoveFeeds /*2.2*/ ) {
    {
        auto feedsConfiguration = createFeedsConfiguration();
        feedsConfiguration->removeFeed( 0 );
        feedsConfiguration->removeFeed( 1 );
        feedsConfiguration->removeFeed( 2 );
        feedsConfiguration->commit();
    }

    auto feedsConfiguration = createFeedsConfiguration();
    EXPECT_EQ( feedsConfiguration->getFeedsCount(), 3 );
    EXPECT_EQ( Feed::toString( feedsConfiguration->getFeed( 0 ) ),
               "src/gz ezlo_openwrt_base "
               "http://dl.mios.com/linux_firmware/dev/bundle/h2.1/v1/latest/system/packages/"
               "arm_cortex-a7_neon-vfpv4/base" );
    EXPECT_EQ( Feed::toString( feedsConfiguration->getFeed( 1 ) ),
               "src/gz ezlo_openwrt_packages "
               "http://dl.mios.com/linux_firmware/dev/bundle/h2.1/v1/latest/system/packages/arm_cortex-a7_neon-vfpv4/"
               "packages" );
    EXPECT_EQ( Feed::toString( feedsConfiguration->getFeed( 2 ) ),
               "src/gz ezlo_openwrt_hub_plugins "
               "http://dl.mios.com/linux_firmware/dev/bundle/h2.1/v1/latest/plugins" );
}

TEST_F( FeedsConfigurationTestSuite, testAddFeeds /*2.3*/ ) {
    {
        auto feedsConfiguration = createFeedsConfiguration();
        feedsConfiguration->clearFeeds();
        feedsConfiguration->addFeed( { DownloadMethod::SrcGz, "name1", "url1" } );
        feedsConfiguration->addFeed( {} );
        feedsConfiguration->addFeed( { DownloadMethod::Src, "name2", "url2" } );
        feedsConfiguration->commit();
    }

    auto feedsConfiguration = createFeedsConfiguration();
    EXPECT_EQ( feedsConfiguration->getFeedsCount(), 2 );
    EXPECT_EQ( Feed::toString( feedsConfiguration->getFeed( 0 ) ), "src/gz name1 url1" );
    EXPECT_EQ( Feed::toString( feedsConfiguration->getFeed( 1 ) ), "src name2 url2" );
}

TEST_F( FeedsConfigurationTestSuite, testModifyFeeds /*2.4*/ ) {
    {
        auto feedsConfiguration = createFeedsConfiguration();

        auto& feed = feedsConfiguration->takeFeed( 2 );
        feed = { DownloadMethod::Src, "new_name", "new_url" };

        feedsConfiguration->commit();
    }

    auto feedsConfiguration = createFeedsConfiguration();
    EXPECT_EQ( feedsConfiguration->getFeedsCount(), 6 );
    EXPECT_EQ( Feed::toString( feedsConfiguration->getFeed( 2 ) ), "src new_name new_url" );
}

TEST_F( FeedsConfigurationTestSuite, testExplicitRevert /*3.1*/ ) {
    {
        auto feedsConfiguration = createFeedsConfiguration();
        feedsConfiguration->clearFeeds();
        feedsConfiguration->revert();
    }

    auto feedsConfiguration = createFeedsConfiguration();
    EXPECT_EQ( feedsConfiguration->getFeedsCount(), 6 );
}

TEST_F( FeedsConfigurationTestSuite, testImplicitRevert /*3.2*/ ) {
    {
        auto feedsConfiguration = createFeedsConfiguration();
        feedsConfiguration->clearFeeds();
    }

    auto feedsConfiguration = createFeedsConfiguration();
    EXPECT_EQ( feedsConfiguration->getFeedsCount(), 6 );
}
