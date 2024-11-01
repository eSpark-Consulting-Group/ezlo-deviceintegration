#include "service_interfaces/strand.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <thread>
#include <future>

namespace HubPlatform {
namespace Services {
namespace UT {

using namespace ::testing;

class StrandTestSuite : public TestWithParam< size_t > {};

TEST_P( StrandTestSuite, dispatchCase ) {
    TaskLoop loop( GetParam() );
    auto runLoopFuture = std::async( std::launch::async, [&] { loop.run(); } );

    auto strand = Strand::createStrand( loop, Strand::generateId() );

    auto threadId = std::this_thread::get_id();
    auto taskDonePromise = std::promise< bool >();
    auto task = [&taskDonePromise, threadId]() {
        // This task should not be called from the thread that posted it,
        // it must be called from one of worker threads.
        EXPECT_FALSE( std::this_thread::get_id() == threadId );
        taskDonePromise.set_value( true );
    };

    strand->dispatch( task );
    EXPECT_TRUE( taskDonePromise.get_future().get() );

    loop.stop();
    runLoopFuture.get();
}

TEST_P( StrandTestSuite, postCase ) {
    TaskLoop loop( GetParam() );
    auto runLoopFuture = std::async( std::launch::async, [&] { loop.run(); } );

    auto strand = Strand::createStrand( loop, Strand::generateId() );

    auto threadId = std::this_thread::get_id();
    auto taskDonePromise = std::promise< bool >();
    auto task = [&]() {
        // This task should not be called from the thread that posted it,
        // it must be called from one of worker threads.
        EXPECT_FALSE( std::this_thread::get_id() == threadId );
        taskDonePromise.set_value( true );
    };

    strand->post( task );
    EXPECT_TRUE( taskDonePromise.get_future().get() );

    loop.stop();
    runLoopFuture.get();
}

TEST_P( StrandTestSuite, dispatchSequentalCase ) {
    TaskLoop loop( GetParam() );
    auto runLoopFuture = std::async( std::launch::async, [&] { loop.run(); } );

    auto strand = Strand::createStrand( loop, Strand::generateId() );

    auto threadId = std::this_thread::get_id();
    auto taskDonePromise = std::promise< bool >();
    auto task = [&]() {
        EXPECT_FALSE( std::this_thread::get_id() == threadId );
        auto nextTaskThreadId = std::this_thread::get_id();
        auto nextTaskDone = std::make_shared< std::atomic_bool >( false );
        auto taskNext = [&taskDonePromise, nextTaskDone, nextTaskThreadId] {
            EXPECT_TRUE( std::this_thread::get_id() == nextTaskThreadId );
            taskDonePromise.set_value( true );
            *nextTaskDone = true;
        };
        strand->dispatch( taskNext );
        // taskNext must be executed immediately
        EXPECT_TRUE( *nextTaskDone );
    };

    strand->dispatch( task );

    EXPECT_TRUE( taskDonePromise.get_future().get() );

    loop.stop();
    runLoopFuture.get();
}

TEST_P( StrandTestSuite, postSequentalCase ) {
    TaskLoop loop( GetParam() );
    auto runLoopFuture = std::async( std::launch::async, [&] { loop.run(); } );

    auto strand = Strand::createStrand( loop, Strand::generateId() );

    auto taskDonePromise = std::promise< bool >();
    auto task = [&]() {
        auto nextTaskDone = std::make_shared< std::atomic_bool >( false );
        auto taskNext = [&taskDonePromise, nextTaskDone] {
            taskDonePromise.set_value( true );
            *nextTaskDone = true;
        };
        strand->post( taskNext );
        // taskNext must be queued and executed later
        EXPECT_FALSE( *nextTaskDone );
    };

    strand->post( task );

    EXPECT_TRUE( taskDonePromise.get_future().get() );

    loop.stop();
    runLoopFuture.get();
}

TEST_P( StrandTestSuite, strandDestroyBeforeDoneCase ) {
    TaskLoop loop( GetParam() );
    auto runLoopFuture = std::async( std::launch::async, [&] { loop.run(); } );

    auto strand = Strand::createStrand( loop, Strand::generateId() );

    constexpr int cycleCount = 4;
    int counter = 0;
    auto taskStartedPromise = std::promise< bool >();
    auto task = [&] {
        auto counterTask = [&]() {
            std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
            ++counter;
        };

        for( int i = 0; i < cycleCount; ++i ) {
            strand->post( counterTask );
        }
        taskStartedPromise.set_value( true );
    };

    strand->post( task );
    EXPECT_TRUE( taskStartedPromise.get_future().get() );
    strand.reset();

    loop.stop();
    runLoopFuture.get();

    EXPECT_EQ( counter, cycleCount );
}

INSTANTIATE_TEST_CASE_P( DiffThreadsCount, StrandTestSuite, ::testing::Values( 0, 1, 3, 8, 12 ) );

}  // namespace UT
}  // namespace Services
}  // namespace HubPlatform
