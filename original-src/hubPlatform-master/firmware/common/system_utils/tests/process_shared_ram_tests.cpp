#include "system_utils/process/shared_ram.h"
#include "system_utils/process/forker.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <future>
#include <fstream>
#include <unistd.h>

/*

TESTPLAN

DONE    1. Shared RAM between processes
DONE        1.1 read/write Shared RAM within sindle process
DONE        1.2 read/write Shared RAM within different processes
DONE        1.3 read/write Shared RAM within sindle processes

*/

namespace HubPlatform {
namespace Process {
namespace UT {

const static std::chrono::seconds WaitTimeoutSec = std::chrono::seconds( 5 );

class SharedRamTests : public ::testing::Test {
public:
    void TearDown() override {
        SharedRam sharedRam( getId(), sizeof( int ) );
        sharedRam.release();
    }

    const std::string& getId() {
        static const std::string id( std::to_string( getpid() ) );
        return id;
    }
};

TEST_F( SharedRamTests, readWriteInSingleProcess ) {  // 1.1
    SharedRam sharedRam1( getId(), sizeof( int ) );
    ASSERT_NE( sharedRam1.get(), nullptr );
    ASSERT_EQ( sharedRam1.size(), sizeof( int ) );

    SharedRam sharedRam2( getId(), sizeof( int ) );
    ASSERT_NE( sharedRam2.get(), nullptr );
    ASSERT_EQ( sharedRam2.size(), sizeof( int ) );

    auto value1 = static_cast< int* >( sharedRam1.get() );
    auto value2 = static_cast< int* >( sharedRam2.get() );

    *value1 = 1;
    EXPECT_EQ( *value2, 1 );

    *value1 = 23;
    EXPECT_EQ( *value2, 23 );

    *value2 = 33;
    EXPECT_EQ( *value1, 33 );

    *value2 = 44;
    EXPECT_EQ( *value1, 44 );
}

TEST_F( SharedRamTests, readWriteInDifferentProcesses1 ) {  // 1.2
    using Status = Process::Forker::Status;

    SharedRam sharedRam( getId(), sizeof( int ) );
    ASSERT_NE( sharedRam.get(), nullptr );
    ASSERT_EQ( sharedRam.size(), sizeof( int ) );
    auto value = static_cast< int* >( sharedRam.get() );
    *value = 1;

    auto id = getId();
    auto job = [id]( const std::string& errorsOutputFile ) {
        try {
            SharedRam sharedRam( id, sizeof( int ) );
            auto value = static_cast< int* >( sharedRam.get() );
            *value = 100;
        }
        catch( const std::exception& ex ) {
            std::ofstream errorFile( errorsOutputFile );
            errorFile << ex.what() << std::endl;
        }
    };

    auto promise1 = std::make_shared< std::promise< std::string > >();
    Process::Forker::Instance processForkerInstance = Process::Forker::getInstance();
    Process::Forker& processForker = *processForkerInstance;
    processForker.fork( job,
                        [promise1]( pid_t /*pid*/, Status /*status*/, int /*exitCode*/, const std::string& error ) {
                            promise1->set_value( error );
                        } );

    auto future1 = promise1->get_future();
    auto futureStatus = future1.wait_for( WaitTimeoutSec );
    ASSERT_EQ( futureStatus, std::future_status::ready );
    auto error = future1.get();
    EXPECT_TRUE( error.empty() ) << error;
    EXPECT_EQ( *value, 100 );
}

TEST_F( SharedRamTests, readWriteInDifferentProcesses2 ) {  // 1.3
    using Status = Process::Forker::Status;

    auto id = getId();
    auto job = [id]( const std::string& errorsOutputFile ) {
        try {
            SharedRam sharedRam( id, sizeof( int ) );
            auto value = static_cast< int* >( sharedRam.get() );
            *value = 100;
        }
        catch( const std::exception& ex ) {
            std::ofstream errorFile( errorsOutputFile );
            errorFile << ex.what() << std::endl;
        }
    };

    auto promise1 = std::make_shared< std::promise< std::string > >();
    Process::Forker::Instance processForkerInstance = Process::Forker::getInstance();
    Process::Forker& processForker = *processForkerInstance;
    processForker.fork( job,
                        [promise1]( pid_t /*pid*/, Status /*status*/, int /*exitCode*/, const std::string& error ) {
                            promise1->set_value( error );
                        } );

    auto future1 = promise1->get_future();
    auto futureStatus = future1.wait_for( WaitTimeoutSec );
    ASSERT_EQ( futureStatus, std::future_status::ready );
    auto error = future1.get();
    EXPECT_TRUE( error.empty() ) << error;

    SharedRam sharedRam( getId(), sizeof( int ) );
    ASSERT_NE( sharedRam.get(), nullptr );
    ASSERT_EQ( sharedRam.size(), sizeof( int ) );
    auto value = static_cast< int* >( sharedRam.get() );
    EXPECT_EQ( *value, 100 );
}

}  // namespace UT
}  // namespace Process
}  // namespace HubPlatform
