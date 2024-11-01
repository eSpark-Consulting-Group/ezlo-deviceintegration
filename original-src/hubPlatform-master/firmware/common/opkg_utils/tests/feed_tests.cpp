#include "opkg_utils/feeds/feed.h"

#include "bus/messagesclasses.h"
#include "json_utils/json_utils.h"
#include "log_utils/ezlo_log.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

/*

TESTPLAN

DONE    1 Conversion from string
DONE        1.1 Correct src/gz feed
DONE        1.2 Correct src feed
DONE        1.3 Incorrect download method
DONE        1.4 Not enough fields
DONE        1.5 Too many fields
DONE    2 Conversion to string
DONE        2.1 Correct src/gz feed
DONE        2.2 Correct src feed
DONE        2.3 Incorrect feed

*/

using namespace ::testing;
using namespace HubPlatform::Opkg;

class FeedTestSuite : public Test {
public:
    FeedTestSuite() {
        // Do not print errors and warnings on erroneous test cases
        // TODO: implement fake log and check error messages too.
        Log::setLocalLogLevel( Log::FATAL );
    }
};

TEST_F( FeedTestSuite, testFromStringCorrectSrcGzFeed /*1.1*/ ) {
    auto feed = Feed::fromString( "src/gz openwrt_core http://store.ezlo.lan/openwrt/targets/sunxi/cortexa7/packages" );

    EXPECT_TRUE( feed.isValid() );
    EXPECT_EQ( feed.downloadMethod, DownloadMethod::SrcGz );
    EXPECT_EQ( feed.name, "openwrt_core" );
    EXPECT_EQ( feed.url, "http://store.ezlo.lan/openwrt/targets/sunxi/cortexa7/packages" );
}

TEST_F( FeedTestSuite, testFromStringCorrectSrcFeed /*1.2*/ ) {
    auto feed = Feed::fromString( "src name url" );

    EXPECT_TRUE( feed.isValid() );
    EXPECT_EQ( feed.downloadMethod, DownloadMethod::Src );
    EXPECT_EQ( feed.name, "name" );
    EXPECT_EQ( feed.url, "url" );
}

TEST_F( FeedTestSuite, testFromStringIncorrectDownloadMethod /*1.3*/ ) {
    auto feed = Feed::fromString( "tar/gz name url" );

    EXPECT_FALSE( feed.isValid() );
    EXPECT_EQ( feed.downloadMethod, DownloadMethod::Unknown );
    EXPECT_EQ( feed.name, "" );
    EXPECT_EQ( feed.url, "" );
}

TEST_F( FeedTestSuite, testFromStringNotEnoughFields /*1.4*/ ) {
    auto feed = Feed::fromString( "src name" );

    EXPECT_FALSE( feed.isValid() );
    EXPECT_EQ( feed.downloadMethod, DownloadMethod::Unknown );
    EXPECT_EQ( feed.name, "" );
    EXPECT_EQ( feed.url, "" );
}

TEST_F( FeedTestSuite, testFromStringTooManyFields /*1.5*/ ) {
    auto feed = Feed::fromString( "src name url comment" );

    EXPECT_FALSE( feed.isValid() );
    EXPECT_EQ( feed.downloadMethod, DownloadMethod::Unknown );
    EXPECT_EQ( feed.name, "" );
    EXPECT_EQ( feed.url, "" );
}

TEST_F( FeedTestSuite, testToStringCorrectSrcGzFeed /*2.1*/ ) {
    Feed feed = {
        DownloadMethod::SrcGz, "openwrt_core", "http://store.ezlo.lan/openwrt/targets/sunxi/cortexa7/packages"
    };

    EXPECT_EQ( Feed::toString( feed ),
               "src/gz openwrt_core http://store.ezlo.lan/openwrt/targets/sunxi/cortexa7/packages" );
}

TEST_F( FeedTestSuite, testToStringCorrectSrcFeed /*2.2*/ ) {
    Feed feed = { DownloadMethod::Src, "name", "url" };

    EXPECT_EQ( Feed::toString( feed ), "src name url" );
}

TEST_F( FeedTestSuite, testToStringIncorrectFeed /*2.2*/ ) {
    Feed feed = { DownloadMethod::Unknown, "any", "any" };

    EXPECT_EQ( Feed::toString( feed ), "" );
}
