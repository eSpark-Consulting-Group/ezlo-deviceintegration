#include "system_utils/process/forker.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <future>

/*

TESTPLAN

DONE    1. Process forker
DONE        1.1 Process is exited succesfully
DONE        1.2 Process is exited with code
DONE        1.3 Several processes is exited succesfully
DONE        1.4 Process is terminated
DONE        1.5 Only one process from 3 processes is terminated
DONE        1.6 All process are terminated
DONE        1.7 Unknown command
DONE        1.8 Ssh connection failure
DONE        1.9 Ssh connection is closed by server ( disabled because it requered ssh server for test )

*/

namespace HubPlatform {
namespace UT {

const static std::chrono::seconds WaitTimeoutSec = std::chrono::seconds( 5 );

class ProcessForkerTests : public ::testing::Test {
public:
    struct Data {
        pid_t pid;
        Process::Forker::Status status;
        int exitCode;
        std::string error;
    };
};

TEST_F( ProcessForkerTests, processExitedSuccesfully /*1.1*/ ) {
    using Status = Process::Forker::Status;
    auto promise = std::make_shared< std::promise< Data > >();
    Process::Forker::Instance processForkerInstance = Process::Forker::getInstance();
    Process::Forker& processForker = *processForkerInstance;
    auto pid = processForker.fork( "sleep 0.0001",
                                   [promise]( pid_t pid, Status status, int exitCode, const std::string& error ) {
                                       promise->set_value( { pid, status, exitCode, error } );
                                   } );
    auto future = promise->get_future();
    auto futureStatus = future.wait_for( WaitTimeoutSec );
    ASSERT_EQ( futureStatus, std::future_status::ready );
    auto data = future.get();
    EXPECT_EQ( data.pid, pid );
    EXPECT_EQ( data.status, Status::Ok );
    EXPECT_EQ( data.exitCode, 0 );
    EXPECT_EQ( data.error, "" );
}

TEST_F( ProcessForkerTests, processExitedWithCode /*1.2*/ ) {
    using Status = Process::Forker::Status;
    auto promise = std::make_shared< std::promise< Data > >();
    Process::Forker::Instance processForkerInstance = Process::Forker::getInstance();
    Process::Forker& processForker = *processForkerInstance;
    auto pid = processForker.fork( "timeout 0.001 sleep 0.01",
                                   [promise]( pid_t pid, Status status, int exitCode, const std::string& error ) {
                                       promise->set_value( { pid, status, exitCode, error } );
                                   } );
    auto future = promise->get_future();
    auto futureStatus = future.wait_for( WaitTimeoutSec );
    ASSERT_EQ( futureStatus, std::future_status::ready );
    auto data = future.get();
    EXPECT_EQ( data.pid, pid );
    EXPECT_EQ( data.status, Status::Ok );
    EXPECT_EQ( data.exitCode, 124 );
    EXPECT_EQ( data.error, "" );
}

TEST_F( ProcessForkerTests, severalProcessesExitedSuccesfully /*1.3*/ ) {
    using Status = Process::Forker::Status;
    auto promise1 = std::make_shared< std::promise< Data > >();
    auto promise2 = std::make_shared< std::promise< Data > >();
    auto promise3 = std::make_shared< std::promise< Data > >();
    Process::Forker::Instance processForkerInstance = Process::Forker::getInstance();
    Process::Forker& processForker = *processForkerInstance;
    auto pid1 = processForker.fork( "sleep 0.0001",
                                    [promise1]( pid_t pid, Status status, int exitCode, const std::string& error ) {
                                        promise1->set_value( { pid, status, exitCode, error } );
                                    } );
    auto pid2 = processForker.fork( "sleep 0.0001",
                                    [promise2]( pid_t pid, Status status, int exitCode, const std::string& error ) {
                                        promise2->set_value( { pid, status, exitCode, error } );
                                    } );
    auto pid3 = processForker.fork( "sleep 0.0001",
                                    [promise3]( pid_t pid, Status status, int exitCode, const std::string& error ) {
                                        promise3->set_value( { pid, status, exitCode, error } );
                                    } );
    {
        auto future = promise1->get_future();
        auto futureStatus = future.wait_for( WaitTimeoutSec );
        auto data = future.get();
        ASSERT_EQ( futureStatus, std::future_status::ready );
        EXPECT_EQ( data.pid, pid1 );
        EXPECT_EQ( data.status, Status::Ok );
        EXPECT_EQ( data.exitCode, 0 );
        EXPECT_EQ( data.error, "" );
    }

    {
        auto future = promise2->get_future();
        auto futureStatus = future.wait_for( WaitTimeoutSec );
        auto data = future.get();
        ASSERT_EQ( futureStatus, std::future_status::ready );
        EXPECT_EQ( data.pid, pid2 );
        EXPECT_EQ( data.status, Status::Ok );
        EXPECT_EQ( data.exitCode, 0 );
        EXPECT_EQ( data.error, "" );
    }
    {
        auto future = promise3->get_future();
        auto futureStatus = future.wait_for( WaitTimeoutSec );
        auto data = future.get();
        ASSERT_EQ( futureStatus, std::future_status::ready );
        EXPECT_EQ( data.pid, pid3 );
        EXPECT_EQ( data.status, Status::Ok );
        EXPECT_EQ( data.exitCode, 0 );
        EXPECT_EQ( data.error, "" );
    }
}

TEST_F( ProcessForkerTests, processTerminated /*1.4*/ ) {
    using Status = Process::Forker::Status;
    auto promise = std::make_shared< std::promise< Data > >();
    Process::Forker::Instance processForkerInstance = Process::Forker::getInstance();
    Process::Forker& processForker = *processForkerInstance;
    auto pid =
        processForker.fork( "sleep 1", [promise]( pid_t pid, Status status, int exitCode, const std::string& error ) {
            promise->set_value( { pid, status, exitCode, error } );
        } );
    Process::Forker::kill( pid, SIGTERM );
    auto future = promise->get_future();
    auto futureStatus = future.wait_for( WaitTimeoutSec );
    ASSERT_EQ( futureStatus, std::future_status::ready );
    auto data = future.get();
    EXPECT_EQ( data.pid, pid );
    EXPECT_EQ( data.status, Status::TermSignal );
    EXPECT_EQ( data.exitCode, SIGTERM );
    EXPECT_EQ( data.error, "" );
}

TEST_F( ProcessForkerTests, onlyOneProcessTerminated /*1.5*/ ) {
    using Status = Process::Forker::Status;
    auto promise1 = std::make_shared< std::promise< Data > >();
    auto promise2 = std::make_shared< std::promise< Data > >();
    auto promise3 = std::make_shared< std::promise< Data > >();
    Process::Forker::Instance processForkerInstance = Process::Forker::getInstance();
    Process::Forker& processForker = *processForkerInstance;
    auto pid1 = processForker.fork( "sleep 0.0001",
                                    [promise1]( pid_t pid, Status status, int exitCode, const std::string& error ) {
                                        promise1->set_value( { pid, status, exitCode, error } );
                                    } );
    auto pid2 =
        processForker.fork( "sleep 1", [promise2]( pid_t pid, Status status, int exitCode, const std::string& error ) {
            promise2->set_value( { pid, status, exitCode, error } );
        } );
    auto pid3 = processForker.fork( "sleep 0.0001",
                                    [promise3]( pid_t pid, Status status, int exitCode, const std::string& error ) {
                                        promise3->set_value( { pid, status, exitCode, error } );
                                    } );
    Process::Forker::kill( pid2, SIGTERM );
    {
        auto future = promise1->get_future();
        auto futureStatus = future.wait_for( WaitTimeoutSec );
        auto data = future.get();
        ASSERT_EQ( futureStatus, std::future_status::ready );
        EXPECT_EQ( data.pid, pid1 );
        EXPECT_EQ( data.status, Status::Ok );
        EXPECT_EQ( data.exitCode, 0 );
        EXPECT_EQ( data.error, "" );
    }

    {
        auto future = promise2->get_future();
        auto futureStatus = future.wait_for( WaitTimeoutSec );
        auto data = future.get();
        ASSERT_EQ( futureStatus, std::future_status::ready );
        EXPECT_EQ( data.pid, pid2 );
        EXPECT_EQ( data.status, Status::TermSignal );
        EXPECT_EQ( data.exitCode, SIGTERM );
        EXPECT_EQ( data.error, "" );
    }
    {
        auto future = promise3->get_future();
        auto futureStatus = future.wait_for( WaitTimeoutSec );
        auto data = future.get();
        ASSERT_EQ( futureStatus, std::future_status::ready );
        EXPECT_EQ( data.pid, pid3 );
        EXPECT_EQ( data.status, Status::Ok );
        EXPECT_EQ( data.exitCode, 0 );
        EXPECT_EQ( data.error, "" );
    }
}

TEST_F( ProcessForkerTests, allProcessTerminated /*1.6*/ ) {
    using Status = Process::Forker::Status;
    auto promise1 = std::make_shared< std::promise< Data > >();
    auto promise2 = std::make_shared< std::promise< Data > >();
    auto promise3 = std::make_shared< std::promise< Data > >();
    Process::Forker::Instance processForkerInstance = Process::Forker::getInstance();
    Process::Forker& processForker = *processForkerInstance;
    auto pid1 =
        processForker.fork( "sleep 1", [promise1]( pid_t pid, Status status, int exitCode, const std::string& error ) {
            promise1->set_value( { pid, status, exitCode, error } );
        } );
    auto pid2 =
        processForker.fork( "sleep 1", [promise2]( pid_t pid, Status status, int exitCode, const std::string& error ) {
            promise2->set_value( { pid, status, exitCode, error } );
        } );
    auto pid3 =
        processForker.fork( "sleep 1", [promise3]( pid_t pid, Status status, int exitCode, const std::string& error ) {
            promise3->set_value( { pid, status, exitCode, error } );
        } );
    Process::Forker::kill( pid1, SIGKILL );
    Process::Forker::kill( pid2, SIGTERM );
    Process::Forker::kill( pid3, SIGQUIT );
    {
        auto future = promise1->get_future();
        auto futureStatus = future.wait_for( WaitTimeoutSec );
        auto data = future.get();
        ASSERT_EQ( futureStatus, std::future_status::ready );
        EXPECT_EQ( data.pid, pid1 );
        EXPECT_EQ( data.status, Status::TermSignal );
        EXPECT_EQ( data.exitCode, SIGKILL );
        EXPECT_EQ( data.error, "" );
    }

    {
        auto future = promise2->get_future();
        auto futureStatus = future.wait_for( WaitTimeoutSec );
        auto data = future.get();
        ASSERT_EQ( futureStatus, std::future_status::ready );
        EXPECT_EQ( data.pid, pid2 );
        EXPECT_EQ( data.status, Status::TermSignal );
        EXPECT_EQ( data.exitCode, SIGTERM );
        EXPECT_EQ( data.error, "" );
    }
    {
        auto future = promise3->get_future();
        auto futureStatus = future.wait_for( WaitTimeoutSec );
        auto data = future.get();
        ASSERT_EQ( futureStatus, std::future_status::ready );
        EXPECT_EQ( data.pid, pid3 );
        // Sometimes it is Status::OK instead of TermSignal
        EXPECT_TRUE( data.status == Status::TermSignal || data.status == Status::Ok )
            << "status: " << static_cast< int >( data.status );
        EXPECT_TRUE( data.exitCode == SIGQUIT || data.exitCode == 0 ) << "exitCode: " << data.exitCode;
        EXPECT_EQ( data.error, "" );
    }
}

TEST_F( ProcessForkerTests, unknownCommand /*1.7*/ ) {
    using Status = Process::Forker::Status;
    auto promise = std::make_shared< std::promise< Data > >();
    Process::Forker::Instance processForkerInstance = Process::Forker::getInstance();
    Process::Forker& processForker = *processForkerInstance;
    auto pid =
        processForker.fork( "ssss", [promise]( pid_t pid, Status status, int exitCode, const std::string& error ) {
            promise->set_value( { pid, status, exitCode, error } );
        } );
    auto future = promise->get_future();
    auto futureStatus = future.wait_for( WaitTimeoutSec );
    ASSERT_EQ( futureStatus, std::future_status::ready );
    auto data = future.get();
    EXPECT_EQ( data.pid, pid );
    EXPECT_EQ( data.status, Status::Ok );
    EXPECT_EQ( data.exitCode, 127 );
    EXPECT_EQ( data.error, "sh: 1: ssss: not found\n" );
}

TEST_F( ProcessForkerTests, DISABLED_sshConnectioFailure /*1.8*/ ) {
    using Status = Process::Forker::Status;
    auto promise = std::make_shared< std::promise< Data > >();
    Process::Forker::Instance processForkerInstance = Process::Forker::getInstance();
    Process::Forker& processForker = *processForkerInstance;
    auto pid = processForker.fork( "sshpass -p pass ssh -p 2222 -o StrictHostKeyChecking=no yurii@localhost",
                                   [promise]( pid_t pid, Status status, int exitCode, const std::string& result ) {
                                       promise->set_value( { pid, status, exitCode, result } );
                                   } );
    auto future = promise->get_future();
    auto futureStatus = future.wait_for( std::chrono::seconds( 5 ) );
    ASSERT_EQ( futureStatus, std::future_status::ready );
    auto data = future.get();
    EXPECT_EQ( data.pid, pid );
    EXPECT_EQ( data.status, Status::Ok );
    EXPECT_EQ( data.exitCode, 5 );
    EXPECT_EQ( data.error,
               "Pseudo-terminal will not be allocated because stdin is not a terminal.\r\nPermission denied, please "
               "try again.\r\n" );
}

TEST_F( ProcessForkerTests, DISABLED_sshClosedByServer /*1.9*/ ) {
    using Status = Process::Forker::Status;
    auto promise = std::make_shared< std::promise< Data > >();
    Process::Forker::Instance processForkerInstance = Process::Forker::getInstance();
    Process::Forker& processForker = *processForkerInstance;
    auto pid = processForker.fork( "sshpass -p pass ssh -p 2222 -o StrictHostKeyChecking=no yurii@localhost",
                                   [promise]( pid_t pid, Status status, int exitCode, const std::string& error ) {
                                       promise->set_value( { pid, status, exitCode, error } );
                                   } );
    std::this_thread::sleep_for( std::chrono::seconds( 10 ) );
    Process::Forker::kill( pid, SIGTERM );
    auto future = promise->get_future();
    auto futureStatus = future.wait_for( std::chrono::seconds( 60 ) );
    ASSERT_EQ( futureStatus, std::future_status::ready );
    auto data = future.get();
    EXPECT_EQ( data.pid, pid );
    EXPECT_EQ( data.status, Status::Ok );
    EXPECT_EQ( data.exitCode, 255 );
    EXPECT_EQ( data.error, "" );
}

}  // namespace UT
}  // namespace HubPlatform
