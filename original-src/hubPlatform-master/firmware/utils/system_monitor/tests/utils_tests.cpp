#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "system_monitor/utils.h"

#include "base_utils/utils.h"
#include "base_utils/string_utils.h"
#include "system_utils/system.h"

#include "test_utils/temporary_folder_wrapper.h"

#include <thread>
#include <chrono>

#include <unistd.h>

/*

TESTPLAN

DONE    1. SystemMonitor utils tests
DONE        1.0 Create and kill process
DONE        1.1 Store and load process PID
DONE        1.2 Add log header
*/

namespace HubPlatform {
namespace UT {

class SystemMonitorUtilsTests : public ::testing::Test {};

TEST_F( SystemMonitorUtilsTests, createAndKillProcess /*1.0*/ ) {
    using namespace HubPlatform::SystemMonitor;

    HubPlatform::Tests::TemporaryFolderWrapper tmpDir;
    const std::string logfile = tmpDir.getPath() + "/logfile.log";

    auto pid = Utils::launchCommand( "echo executed >> " + logfile + " ; sleep 10" );
    std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
    ASSERT_GT( pid, 0 );

    EXPECT_TRUE( Utils::isProcessRunning( pid ) );

    Utils::sendTerminateSignal( pid );

    EXPECT_TRUE( waitForCondition( [pid]() { return Utils::isProcessRunning( pid ); },
                                   std::chrono::seconds( 5 ),
                                   std::chrono::milliseconds( 200 ) ) );

    EXPECT_EQ( StringUtils::countTokens( System::readFile( logfile ), "executed" ), 1 );
}

TEST_F( SystemMonitorUtilsTests, storeAndLoadProcessPid /*1.1*/ ) {
    using namespace HubPlatform::SystemMonitor;

    HubPlatform::Tests::TemporaryFolderWrapper tmpDir;

    const int pid = getpid();

    std::string pidfile = tmpDir.getPath() + "/pidfile.pid";

    Utils::storeProcessPid( pid, pidfile );

    EXPECT_EQ( pid, Utils::loadProcessPid( pidfile ) );

    EXPECT_EQ( 0, Utils::loadProcessPid( "" ) );
    EXPECT_EQ( 0, Utils::loadProcessPid( tmpDir.getPath() + "/nonexisting" ) );
}

TEST_F( SystemMonitorUtilsTests, addLogHeader /*1.2*/ ) {
    using namespace HubPlatform::SystemMonitor;

    HubPlatform::Tests::TemporaryFolderWrapper tmpDir;

    const int pid = getpid();
    std::string logfile = tmpDir.getPath() + "/logfile.log";

    Utils::addLogHeader( logfile );
    EXPECT_EQ( StringUtils::countTokens( System::readFile( logfile ), "STARTED" ), 1 );

    Utils::addLogHeader( logfile );
    EXPECT_EQ( StringUtils::countTokens( System::readFile( logfile ), "STARTED" ), 2 );
}

}  // namespace UT
}  // namespace HubPlatform
