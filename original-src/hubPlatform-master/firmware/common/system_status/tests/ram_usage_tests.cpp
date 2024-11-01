#include <gtest/gtest.h>
#include "system_status/ram_usage.h"
#include <sstream>

namespace HubPlatform {
namespace UT {

class SystemStatusTests : public ::testing::Test {};

TEST_F( SystemStatusTests, processStatusLineParserToMap ) {
    std::stringstream lines;
    {
        lines << "Name:\tprocess_name" << std::endl;
        lines << "Pid:\t123" << std::endl;
        lines << "RssAnon:\t96 kB" << std::endl;
        lines << "VmRss:\t17000 kB" << std::endl;
    }

    auto parameters = HubPlatform::SystemUtils::ProcFS::MemoryInfo::parseProcStatus( lines );
    EXPECT_EQ( parameters[ "Name" ], "process_name" );
    EXPECT_EQ( parameters[ "Pid" ], "123" );
    EXPECT_EQ( parameters[ "RssAnon" ], "96 kB" );
    EXPECT_EQ( parameters[ "VmRss" ], "17000 kB" );
}

TEST_F( SystemStatusTests, fetchStatusBySelf ) {
    using namespace HubPlatform::SystemUtils::ProcFS;
    MemoryInfo memoryInfo;
    bool callStatus = false;
    MemoryInfo::ProcessStatus processStatus;

    std::tie( callStatus, processStatus ) = memoryInfo.fetchProcessMemoryUsage( "self" );
    ASSERT_TRUE( callStatus );
    EXPECT_EQ( processStatus.name, "system_status_tests" );
    EXPECT_GT( processStatus.rssAnon, 0 );
    EXPECT_GT( processStatus.memoryTotal, 0 );
}

TEST_F( SystemStatusTests, fetchStatusByPid ) {
    pid_t selfPid = getpid();

    using namespace HubPlatform::SystemUtils::ProcFS;
    MemoryInfo memoryInfo;
    bool callStatus = false;
    MemoryInfo::ProcessStatus processStatus;

    std::tie( callStatus, processStatus ) = memoryInfo.fetchProcessMemoryUsage( std::to_string( selfPid ).c_str() );
    ASSERT_TRUE( callStatus );
    EXPECT_EQ( processStatus.name, "system_status_tests" );
    EXPECT_GT( processStatus.rssAnon, 0 );
    EXPECT_GT( processStatus.memoryTotal, 0 );
}

TEST_F( SystemStatusTests, processDoesNotExist ) {
    using namespace HubPlatform::SystemUtils::ProcFS;
    MemoryInfo memoryInfo;
    bool callStatus = false;

    std::tie( callStatus, std::ignore ) = memoryInfo.fetchProcessMemoryUsage( "process-pid-doesnt-exist" );
    ASSERT_FALSE( callStatus );
}

}  // namespace UT
}  // namespace HubPlatform