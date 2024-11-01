#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "system_monitor/process.h"
#include "system_monitor/utils.h"

#include "base_utils/utils.h"
#include "base_utils/string_utils.h"
#include "system_utils/system.h"
#include "test_utils/temporary_folder_wrapper.h"

#include <thread>
#include <chrono>

/*

TESTPLAN

DONE    1. Process tests
DONE        1.0 Run repeatable process
DONE        1.1 Stop process between iterations
DONE        1.2 Stop running process
*/

namespace HubPlatform {
namespace UT {

class SystemMonitorProcessTests : public ::testing::Test {};

TEST_F( SystemMonitorProcessTests, runRepeatableProcess /*1.0*/ ) {
    using namespace HubPlatform::SystemMonitor;
    HubPlatform::Tests::TemporaryFolderWrapper tmpDir;

    std::string logfile = tmpDir.getPath() + "/logfile.pid";
    std::string pidfile = tmpDir.getPath() + "/pidfile.pid";
    const std::chrono::seconds repeatInterval( 1 );
    int processPid = 0;

    Process process( "( echo stdout_msg ; echo stderr_msg >&2 ; sleep 1 )", pidfile, logfile, repeatInterval );

    // wait process to start
    EXPECT_TRUE( waitForCondition(
        [&process]() { return process.isRunning(); }, std::chrono::seconds( 5 ), std::chrono::milliseconds( 50 ) ) );
    processPid = process.getPid();
    EXPECT_GT( processPid, 0 );
    EXPECT_EQ( processPid, Utils::loadProcessPid( pidfile ) );

    // wait process to finish
    EXPECT_TRUE( waitForCondition(
        [&process]() { return !process.isRunning(); }, std::chrono::seconds( 5 ), std::chrono::milliseconds( 50 ) ) );
    EXPECT_EQ( process.getPid(), 0 );

    // wait process to restart
    EXPECT_TRUE( waitForCondition(
        [&process]() { return process.isRunning(); }, std::chrono::seconds( 5 ), std::chrono::milliseconds( 50 ) ) );
    EXPECT_GT( process.getPid(), processPid ) << "should get new pid";
    processPid = process.getPid();
    EXPECT_EQ( processPid, Utils::loadProcessPid( pidfile ) );

    // wait process to finish
    EXPECT_TRUE( waitForCondition(
        [&process]() { return !process.isRunning(); }, std::chrono::seconds( 5 ), std::chrono::milliseconds( 50 ) ) );
    EXPECT_EQ( process.getPid(), 0 );

    process.stop();
    EXPECT_EQ( process.isRunning(), false );
    EXPECT_EQ( process.getPid(), 0 );

    std::string logfileContent = System::readFile( logfile );
    EXPECT_EQ( StringUtils::countTokens( logfileContent, "STARTED" ), 2 );
    EXPECT_EQ( StringUtils::countTokens( logfileContent, "stdout_msg" ), 2 );
    EXPECT_EQ( StringUtils::countTokens( logfileContent, "stderr_msg" ), 2 );
}

TEST_F( SystemMonitorProcessTests, stopRunningProcess /*1.1*/ ) {
    using namespace HubPlatform::SystemMonitor;
    HubPlatform::Tests::TemporaryFolderWrapper tmpDir;

    std::string logfile = tmpDir.getPath() + "/logfile.pid";
    std::string pidfile = tmpDir.getPath() + "/pidfile.pid";
    const std::chrono::seconds repeatInterval( 0 );
    int processPid = 0;

    Process process( "sleep 2", pidfile, logfile, repeatInterval );

    // wait process to start
    EXPECT_TRUE( waitForCondition(
        [&process]() { return process.isRunning(); }, std::chrono::seconds( 5 ), std::chrono::milliseconds( 50 ) ) );
    processPid = process.getPid();
    EXPECT_GT( processPid, 0 );
    EXPECT_EQ( processPid, Utils::loadProcessPid( pidfile ) );

    process.stop();
    EXPECT_EQ( process.isRunning(), false );
    EXPECT_EQ( process.getPid(), 0 );

    // No restarts
    std::this_thread::sleep_for( std::chrono::milliseconds( 2500 ) );
    EXPECT_EQ( process.isRunning(), false );
    EXPECT_EQ( process.getPid(), 0 );
}

}  // namespace UT
}  // namespace HubPlatform
