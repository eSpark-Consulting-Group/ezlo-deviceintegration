#include "system_utils/process/passive_process_reader.h"

#include "common_utils/cpp14.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <chrono>
#include <vector>
#include <string>

/*

TESTPLAN

DONE    1. Passive process reader tests
DONE        1.1 Read from process

*/

namespace HubPlatform {
namespace UT {

class PassiveProcessReaderTest : public ::testing::Test {};

TEST_F( PassiveProcessReaderTest, readFromProcess /*1.1*/ ) {
    std::string cmd = "echo stdout string1 ; echo stderr string1 >&2 ; ";
    cmd += "echo stdout string2 ; echo stderr string2 >&2 ; ";

    std::string stdoutOutput;
    std::string stderrOutput;
    bool status = false;

    Process::PassiveProcessReader reader( cmd );
    reader.process().wait();
    EXPECT_TRUE( reader.process().isFinished() );
    EXPECT_EQ( reader.process().getExitCode(), 0 );

    status = reader.processStdout( [&stdoutOutput]( const std::string& buff ) {
        stdoutOutput = buff;
        return true;
    } );
    EXPECT_EQ( status, true );

    status = reader.processStderr( [&stderrOutput]( const std::string& buff ) {
        stderrOutput = buff;
        return false;
    } );
    EXPECT_EQ( status, false );

    EXPECT_EQ( stdoutOutput, "stdout string1\nstdout string2\n" );
    EXPECT_EQ( stderrOutput, "stderr string1\nstderr string2\n" );

    EXPECT_NE( reader.findInStdout( "stdout string2" ), std::string::npos );
    EXPECT_NE( reader.findInStderr( "stderr string2" ), std::string::npos );

    EXPECT_EQ( reader.findInStdout( "unknown" ), std::string::npos );
    EXPECT_EQ( reader.findInStderr( "unknown" ), std::string::npos );

    reader.clearStdout();
    reader.clearStderr();

    status = reader.processStdout( [&stdoutOutput]( const std::string& buff ) {
        stdoutOutput = buff;
        return false;
    } );
    EXPECT_EQ( status, false );

    status = reader.processStderr( [&stderrOutput]( const std::string& buff ) {
        stderrOutput = buff;
        return true;
    } );
    EXPECT_EQ( status, true );

    EXPECT_EQ( stdoutOutput, "" );
    EXPECT_EQ( stderrOutput, "" );
}

}  // namespace UT
}  // namespace HubPlatform
