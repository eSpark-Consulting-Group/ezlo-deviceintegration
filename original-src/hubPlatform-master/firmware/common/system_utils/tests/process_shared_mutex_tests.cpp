#include "system_utils/process/shared_mutex.h"
#include "system_utils/process/forker.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <future>
#include <fstream>
#include <unistd.h>

/*

TESTPLAN

DONE    1. Shared RAM between processes
DONE        1.1 lock/unlock within single process
DONE        1.2 lock/unlock within different processes

*/

namespace HubPlatform {
namespace Process {
namespace UT {

const static std::chrono::seconds WaitTimeoutSec = std::chrono::seconds( 5 );

class SharedMutexTests : public ::testing::Test {
public:
    void TearDown() override {
        SharedMutex sharedMutex( getId() );
        sharedMutex.release();
    }

    const std::string& getId() {
        static const std::string id( std::to_string( getpid() ) );
        return id;
    }
};

TEST_F( SharedMutexTests, singleProcess ) {  // 1.1
    SharedMutex sharedMutex( getId() );
    {
        std::lock_guard< SharedMutex > lock( sharedMutex );
        EXPECT_FALSE( sharedMutex.try_lock() );
    }
    EXPECT_TRUE( sharedMutex.try_lock() );
    EXPECT_FALSE( sharedMutex.try_lock() );
    sharedMutex.unlock();
    EXPECT_TRUE( sharedMutex.try_lock() );
    sharedMutex.unlock();
}

TEST_F( SharedMutexTests, differentProcesses ) {  // 1.2
    using Status = Process::Forker::Status;

    SharedMutex sharedMutex( getId() );
    std::unique_lock< SharedMutex > lock( sharedMutex );

    auto id = getId();
    auto job = [id]( const std::string& errorsOutputFile ) {
        try {
            SharedMutex sharedMutex( id );
            std::lock_guard< SharedMutex > lock( sharedMutex );
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
    ASSERT_EQ( futureStatus, std::future_status::timeout );

    lock.unlock();

    futureStatus = future1.wait_for( WaitTimeoutSec );
    ASSERT_EQ( futureStatus, std::future_status::ready );
    auto error = future1.get();
    EXPECT_TRUE( error.empty() ) << error;
}

}  // namespace UT
}  // namespace Process
}  // namespace HubPlatform
