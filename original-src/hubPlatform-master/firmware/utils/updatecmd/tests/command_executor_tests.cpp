//#include "test_utils/temporary_folder_wrapper.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <sstream>
#include <thread>

#include "updatecmd/command_executor.h"

/*

TESTPLAN

DONE    1. CommandExecutor tests
DONE        1.0 Default flow
DONE        1.1 Command failed
DONE        1.2 Command hanged

*/

namespace HubPlatform {
namespace UT {

using namespace HubPlatform::UpdateCommand;

using testing::_;
using testing::Return;

class CommandExecutorTest : public ::testing::Test {};

TEST_F( CommandExecutorTest, DefaultFlow /*1.0*/ ) {
    size_t commandCounter = 0;
    CommandExecutor::Executor fakeExecutor = [&]( const std::string& command ) {
        ++commandCounter;
        return std::make_pair< std::string, int >( "command_output", 0 );
    };

    std::ostringstream logStream;
    CommandExecutor executor( fakeExecutor, &logStream );

    std::vector< std::string > commands{ "command1", "command2" };
    auto status = executor.run( commands, std::chrono::milliseconds( 50 ) );

    EXPECT_EQ( status, CommandExecutor::Status::Ok );
    EXPECT_EQ( commandCounter, 2 );
}

TEST_F( CommandExecutorTest, CommandFailed /*1.1*/ ) {
    size_t commandCounter = 0;
    CommandExecutor::Executor fakeExecutor = [&]( const std::string& command ) {
        ++commandCounter;
        if( commandCounter == 2 )
            return std::make_pair< std::string, int >( "command_failure_output", -1 );
        return std::make_pair< std::string, int >( "command_output", 0 );
    };

    std::ostringstream logStream;
    CommandExecutor executor( fakeExecutor, &logStream );

    std::vector< std::string > commands{ "command1", "command2", "command3" };
    auto status = executor.run( commands, std::chrono::milliseconds( 50 ) );

    EXPECT_EQ( status, CommandExecutor::Status::Failed );
    EXPECT_EQ( commandCounter, 2 );
}

TEST_F( CommandExecutorTest, CommandHanged /*1.2*/ ) {
    size_t commandCounter = 0;
    CommandExecutor::Executor fakeExecutor = [&]( const std::string& command ) {
        ++commandCounter;
        if( commandCounter == 2 ) {
            std::this_thread::sleep_for( std::chrono::milliseconds( 1000 ) );
        }
        return std::make_pair< std::string, int >( "command_output", 0 );
    };

    std::ostringstream logStream;
    CommandExecutor executor( fakeExecutor, &logStream );

    std::vector< std::string > commands{ "command1", "command2", "command3" };
    auto status = executor.run( commands, std::chrono::milliseconds( 20 ) );

    EXPECT_EQ( status, CommandExecutor::Status::Hanged );
    EXPECT_EQ( commandCounter, 2 );
}

}  // namespace UT
}  // namespace HubPlatform
