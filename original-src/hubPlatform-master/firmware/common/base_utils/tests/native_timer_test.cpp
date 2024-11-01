#include "base_utils/native_timer.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <thread>
#include <mutex>
#include <future>

namespace UT {

namespace {
constexpr size_t ExperimentsCountRatio = 1u;  // Set 1000 for deep test
}

// TODO Fix issues and remove "DISABLED_" from name
class DISABLED_NativeTimerTestSuite : public ::testing::Test {};

TEST_F( DISABLED_NativeTimerTestSuite, init ) {
    NativeTimer timer( []() {} );

    EXPECT_FALSE( timer.isRunning() );
    EXPECT_FALSE( timer.isPeriodic() );

    std::chrono::nanoseconds leaved, period;
    timer.getIntervals( &leaved, &period );

    EXPECT_EQ( leaved, std::chrono::nanoseconds() );
    EXPECT_EQ( period, std::chrono::nanoseconds() );
}

TEST_F( DISABLED_NativeTimerTestSuite, singleShot ) {
    std::atomic< size_t > counter{ 0 };

    const auto startTime = std::chrono::steady_clock::now();

    std::mutex mutex;
    std::condition_variable cv;

    NativeTimer timer( [&]( NativeTimer& timer ) {
        EXPECT_LE( std::chrono::steady_clock::now(), startTime + std::chrono::milliseconds( 50 ) );
        EXPECT_EQ( timer.getCounter(), 1 );
        EXPECT_FALSE( timer.isRunning() );
        EXPECT_FALSE( timer.isStopped() );

        {
            std::unique_lock< std::mutex > lock( mutex );
            ++counter;
        }

        cv.notify_all();
    } );

    EXPECT_TRUE( timer.start( std::chrono::seconds( 10 ), true ) );
    EXPECT_TRUE( timer.isRunning() );
    EXPECT_FALSE( timer.isPeriodic() );
    EXPECT_FALSE( timer.isStopped() );
    EXPECT_EQ( timer.getCounter(), 0 );

    ASSERT_TRUE( timer.start( std::chrono::milliseconds( 1 ), true ) );

    {
        std::unique_lock< std::mutex > lock( mutex );
        cv.wait( lock, [&counter]() { return counter == 1u; } );
    }

    EXPECT_EQ( timer.getCounter(), 1u );

    std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );

    EXPECT_EQ( timer.getCounter(), 1 );
    EXPECT_EQ( counter, 1 );
}

TEST_F( DISABLED_NativeTimerTestSuite, destruction ) {
    std::atomic< bool > isRunning{ true };

    {
        NativeTimer timer( [&isRunning]() { EXPECT_TRUE( isRunning ); } );

        EXPECT_TRUE( timer.start( std::chrono::milliseconds( 1 ) ) );
    }

    isRunning = false;

    std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
}

TEST_F( DISABLED_NativeTimerTestSuite, periodic ) {
    std::atomic< size_t > counter{ 0 };

    std::mutex mutex;
    std::condition_variable cv;

    constexpr const size_t TickCount = 5;

    NativeTimer timer( [&]() {
        std::unique_lock< std::mutex > lock( mutex );
        if( ++counter == TickCount ) {
            lock.unlock();
            cv.notify_all();
        }
    } );

    ASSERT_TRUE( timer.start( std::chrono::milliseconds( 1 ) ) );

    EXPECT_TRUE( timer.isRunning() );
    EXPECT_TRUE( timer.isPeriodic() );
    EXPECT_FALSE( timer.isStopped() );

    {
        std::unique_lock< std::mutex > lock( mutex );
        cv.wait( lock, [&counter]() { return counter >= TickCount; } );
    }

    EXPECT_TRUE( timer.isRunning() );
    EXPECT_TRUE( timer.isPeriodic() );
    EXPECT_FALSE( timer.isStopped() );
}

TEST_F( DISABLED_NativeTimerTestSuite, stopInsideCallback ) {
    std::mutex mutex;
    std::condition_variable cv;
    std::atomic< size_t > counter{ 0 };

    NativeTimer timer( [&]( NativeTimer& timer ) {
        EXPECT_TRUE( timer.isRunning() );
        EXPECT_FALSE( timer.isStopped() );
        timer.stop();

        EXPECT_FALSE( timer.isRunning() );
        EXPECT_TRUE( timer.isStopped() );

        {
            std::unique_lock< std::mutex > lock( mutex );
            ++counter;
        }

        cv.notify_all();
    } );

    ASSERT_TRUE( timer.start( std::chrono::milliseconds( 2 ) ) );

    {
        std::unique_lock< std::mutex > lock( mutex );
        cv.wait( lock, [&counter]() { return counter == 1; } );
    }

    std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );

    EXPECT_EQ( timer.getCounter(), 1 );
}

TEST_F( DISABLED_NativeTimerTestSuite, parallelStartAndParallelStop ) {
    std::atomic< bool > isRunning{ true };

    {
        NativeTimer timer( [&]() { EXPECT_TRUE( isRunning ); } );

        constexpr const size_t ThreadsCount = 10;
        constexpr const size_t ExperimentsCount = 1000 * ExperimentsCountRatio;

        for( size_t i = 0; i < ExperimentsCount; ++i ) {
            {
                std::vector< std::future< void > > futures;
                futures.reserve( ThreadsCount );

                for( size_t i = 0; i < ThreadsCount; ++i ) {
                    futures.push_back( std::async( std::launch::async,
                                                   [&timer]() { timer.start( std::chrono::milliseconds( 2 ) ); } ) );
                }
            }

            EXPECT_TRUE( timer.isRunning() );
            EXPECT_TRUE( timer.isPeriodic() );
            EXPECT_FALSE( timer.isStopped() );

            {
                std::vector< std::future< void > > futures;
                futures.reserve( ThreadsCount );

                for( size_t i = 0; i < ThreadsCount; ++i ) {
                    futures.push_back( std::async( std::launch::async, [&timer]() {
                        timer.stop( NativeTimer::StoppingMode::WaitingForThreads );
                    } ) );
                }
            }

            EXPECT_FALSE( timer.isRunning() );
            EXPECT_FALSE( timer.isPeriodic() );
            EXPECT_TRUE( timer.isStopped() );
        }

        isRunning = false;
    }

    std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );  // verify that no unexpected timer
}

TEST_F( DISABLED_NativeTimerTestSuite, parallelStartStop ) {
    NativeTimer timer( []() { FAIL(); } );

    std::atomic< bool > isRunning{ true };

    auto check = [&]() {
        std::chrono::milliseconds first, periodic;
        timer.getIntervals( &first, &periodic );

        EXPECT_TRUE( ( first.count() == 0 && periodic.count() == 0 ) ||
                     ( ( first.count() + 500 ) / 1000 == 1 && periodic.count() == 2000 ) ||
                     ( ( first.count() + 500 ) / 1000 == 4 && periodic.count() == 5000 ) );
    };

    auto future1 = std::async( std::launch::async, [&]() {
        while( isRunning ) {
            timer.stop();
            check();
        }
    } );

    auto future2 = std::async( std::launch::async, [&]() {
        while( isRunning ) {
            timer.start( std::chrono::milliseconds( 1000 ), std::chrono::milliseconds( 2000 ) );
            check();
        }
    } );

    auto future3 = std::async( std::launch::async, [&]() {
        while( isRunning ) {
            timer.start( std::chrono::milliseconds( 4000 ), std::chrono::milliseconds( 5000 ) );
            check();
        }
    } );

    std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) * ExperimentsCountRatio );

    isRunning = false;
}

TEST_F( DISABLED_NativeTimerTestSuite, startStopWithThreadsWaiting_SingleShot ) {
    std::atomic< bool > flag{ false };

    std::atomic< size_t > counter{ 0 };

    auto waitForChange = [&]( volatile bool required ) {
        while( flag != required ) {
            std::this_thread::sleep_for( std::chrono::microseconds( 500 ) );
        }
    };

    NativeTimer timer( [&]( NativeTimer& timer ) {
        flag = true;

        EXPECT_EQ( timer.getCounter(), 1 );

        waitForChange( false );

        std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );

        ++counter;
    } );

    constexpr const size_t ExperimentsCount = 50 * ExperimentsCountRatio;
    constexpr const size_t ThreadCount = 10;

    for( size_t i = 0; i < ExperimentsCount; ++i ) {
        flag = false;
        EXPECT_TRUE( timer.start( std::chrono::milliseconds( 1 ), true ) );

        waitForChange( true );

        EXPECT_EQ( timer.getCounter(), 1 );

        flag = false;

        std::vector< std::future< void > > futures;
        futures.reserve( ThreadCount );

        for( size_t threadIndex = 0; threadIndex < ThreadCount; ++threadIndex ) {
            futures.push_back( std::async( std::launch::async, [&]() {
                timer.stop( NativeTimer::StoppingMode::WaitingForThreads );

                EXPECT_TRUE( timer.isStopped() );
                EXPECT_EQ( timer.getCounter(), 1 );
            } ) );
        }
    }

    EXPECT_EQ( counter, ExperimentsCount );
}

TEST_F( DISABLED_NativeTimerTestSuite, startStopWithPeriodicTimer_ThreadsWaiting_FakeStarts ) {
    std::atomic< bool > isRunning{ true };

    NativeTimer timer( [&]() {
        EXPECT_TRUE( isRunning );

        std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
    } );

    constexpr const size_t ExperimentsCount = 50 * ExperimentsCountRatio;
    constexpr const size_t ThreadCount = 10;

    for( size_t i = 0; i < ExperimentsCount; ++i ) {
        EXPECT_TRUE( timer.start( std::chrono::milliseconds( 1 ) ) );

        std::this_thread::sleep_for( std::chrono::microseconds( 1200 ) );

        std::vector< std::future< void > > futures;
        futures.reserve( ThreadCount );

        for( size_t threadIndex = 0; threadIndex < ThreadCount; ++threadIndex ) {
            futures.push_back( std::async( std::launch::async, [&]() {
                timer.stop( NativeTimer::StoppingMode::WaitingForThreads );

                timer.start( std::chrono::seconds( 10000 ) );  // Fake start
            } ) );
        }
    }

    timer.stop( NativeTimer::StoppingMode::WaitingForThreads );
    isRunning = false;

    std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );  // wait for unexpected timer events
}

// test possible side effects (on parallel delete/create)
TEST_F( DISABLED_NativeTimerTestSuite, severalTimers ) {
    constexpr const size_t ThreadCount = 10;

    std::vector< std::future< void > > futures;
    futures.reserve( ThreadCount );

    std::atomic< bool > isRunning{ true };

    for( size_t i = 0; i < ThreadCount; ++i ) {
        futures.push_back( std::async( std::launch::async, [&isRunning, i]() {
            NativeTimer timer( []() { FAIL(); } );

            auto check = [&timer, i]() {
                std::chrono::milliseconds first, periodic;
                timer.getIntervals( &first, &periodic );
                EXPECT_TRUE( ( periodic == std::chrono::seconds( i + 1 ) &&
                               ( static_cast< size_t >( first.count() ) + 500 ) / 1000 == i + 1 ) ||
                             ( periodic.count() == 0 && first.count() == 0 ) );
            };

            const auto stopsWithWaitingFuture = std::async( std::launch::async, [&timer, &isRunning, &check]() {
                while( isRunning ) {
                    timer.stop( NativeTimer::StoppingMode::WaitingForThreads );
                    check();
                }
            } );

            const auto stopsFuture = std::async( std::launch::async, [&timer, &isRunning, &check]() {
                while( isRunning ) {
                    timer.stop();
                    check();
                }
            } );

            while( isRunning ) {
                timer.start( std::chrono::seconds( i + 1 ), std::chrono::seconds( i + 1 ) );
                check();
            }
        } ) );
    }

    std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) * ExperimentsCountRatio );

    isRunning = false;
}

}  // namespace UT
