#include "watcher/system_log/system_log.h"

#include "log_utils/ezlo_log.h"
#include "common_utils/cpp14.h"
#include "json_utils/json_utils.h"
#include <gtest/gtest.h>

#include <future>

/*

TESTPLAN

DONE    1. SystemLog tests
DONE        1.1 Parse log records

*/

namespace HubPlatform {
namespace UT {

class SystemLogTestSuite : public ::testing::Test {};

TEST_F( SystemLogTestSuite, parseLogRecords ) /* 1.1 */ {
    const std::string cmd = "echo log record1; echo log record2";

    std::promise< void > promise;
    auto future = promise.get_future();

    std::vector< std::string > records;
    Watcher::SystemLog systemLog;

    auto logRecordEvent = [&records, &promise]( const std::string& record ) {
        records.emplace_back( record );
        if( records.size() == 2 )
            promise.set_value();
    };

    systemLog.subscribe( "subscriber1", logRecordEvent );
    systemLog.start( cmd );

    ASSERT_EQ( future.wait_for( std::chrono::seconds( 5 ) ), std::future_status::ready );

    systemLog.unsubscribe( "subscriber1" );
    systemLog.stop();

    ASSERT_EQ( records.size(), 2 );
    EXPECT_EQ( records.at( 0 ), "log record1" );
    EXPECT_EQ( records.at( 1 ), "log record2" );
}

}  // namespace UT
}  // namespace HubPlatform
