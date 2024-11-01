#include "system_utils/process/process_reader.h"

#include "common_utils/cpp14.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <chrono>
#include <vector>
#include <string>

/*

TESTPLAN

DONE    1. Process reader tests
DONE        1.1 Read from process
DONE        1.2 Kill process
DONE        1.3 Exit status

*/

namespace HubPlatform {
namespace UT {

class ProcessReaderTest : public ::testing::Test {};

TEST_F( ProcessReaderTest, readFromProcess /*1.1*/ ) {
    std::string cmd = "echo stdout string1 ; echo stderr string1 >&2 ; ";
    cmd += "echo stdout string2 ; echo stderr string2 >&2 ; ";

    std::string stdoutOutput;
    auto onStdout = [&stdoutOutput]( const char* bytes, size_t n ) { stdoutOutput.append( bytes, n ); };

    std::string stderrOutput;
    auto onStderr = [&stderrOutput]( const char* bytes, size_t n ) { stderrOutput.append( bytes, n ); };

    Process::ProcessReader process( cmd, onStdout, onStderr );
    process.wait();
    EXPECT_TRUE( process.isFinished() );
    EXPECT_EQ( process.getExitCode(), 0 );

    EXPECT_EQ( stdoutOutput, "stdout string1\nstdout string2\n" );
    EXPECT_EQ( stderrOutput, "stderr string1\nstderr string2\n" );
}

TEST_F( ProcessReaderTest, killProcess /*1.2*/ ) {
    std::string cmd = "echo stdout string1 ; sleep 10; echo stdout string2";

    std::string stdoutOutput;
    auto onStdout = [&stdoutOutput]( const char* bytes, size_t n ) { stdoutOutput.append( bytes, n ); };

    std::string stderrOutput;
    auto onStderr = [&stderrOutput]( const char* bytes, size_t n ) { stderrOutput.append( bytes, n ); };

    Process::ProcessReader process( cmd, onStdout, onStderr );
    std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
    EXPECT_FALSE( process.isFinished() );

    process.stop( std::chrono::milliseconds( 100 ) );
    std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
    EXPECT_TRUE( process.isFinished() );

    EXPECT_EQ( stdoutOutput, "stdout string1\n" );
    EXPECT_EQ( stderrOutput, "" );
}

TEST_F( ProcessReaderTest, exitStatus /*1.3*/ ) {
    std::string cmd = "exit 10";

    auto onStdout = []( const char* bytes, size_t n ) {};
    auto onStderr = []( const char* bytes, size_t n ) {};

    Process::ProcessReader process( cmd, onStdout, onStderr );
    process.wait();
    EXPECT_TRUE( process.isFinished() );
    EXPECT_EQ( process.getExitCode(), 10 );
}

}  // namespace UT
}  // namespace HubPlatform
