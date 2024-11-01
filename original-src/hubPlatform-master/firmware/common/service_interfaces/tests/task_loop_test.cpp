#include "service_interfaces/tests/task_loop_test_helper.h"

#include <bitset>

namespace HubPlatform {
namespace Services {
namespace UT {

class TaskLoopTestSuite : public ::testing::TestWithParam< size_t >, public TaskLoopTestHelper {};

TEST_P( TaskLoopTestSuite, simpleTask ) {
    TaskLoop loop( GetParam() );

    TaskLoopTestHelper::Counter counter1;
    TaskLoopTestHelper::Counter counter2;

    loop.addTask( [&counter1]() { ++counter1; } );

    loop.addTask( [&counter2]() { ++counter2; } );

    loop.addTask( [&]() {
        counter1.wait( 1u );
        counter2.wait( 1u );
        EXPECT_EQ( getTasksCount( loop ), 0 );

        loop.addTask( [&]() { loop.stop(); } );
    } );

    EXPECT_EQ( getTasksCount( loop ), 3u );

    loop.run();

    EXPECT_EQ( counter1.get(), 1u );
    EXPECT_EQ( counter2.get(), 1u );
}

TEST_P( TaskLoopTestSuite, periodicTask ) {
    TaskLoop loop( GetParam() );

    TaskLoopTestHelper::Counter counter;

    loop.addTask(
        [&]() {
            ++counter;

            if( counter.get() == 100u ) {
                loop.stop();
            }
        },
        true );

    loop.run();

    if( GetParam() == 0 ) {
        EXPECT_EQ( counter.get(), 100u );
    }
}

TEST_P( TaskLoopTestSuite, threadsAndSimpleTasks ) {
    TaskLoop loop( GetParam() );

    TaskLoopTestHelper::Counter counter;

    const auto threadsCountWithMain = GetParam() + 1;

    auto future = runInThread( loop );

    for( size_t i = 0; i < threadsCountWithMain; ++i ) {
        loop.addTask( [&]() {
            ++counter;

            counter.wait( threadsCountWithMain + 1 );

            loop.stop();
        } );
    }

    counter.wait( threadsCountWithMain );

    ++counter;
}

TEST_P( TaskLoopTestSuite, threadsAndPostponedTasks ) {
    TaskLoop loop( GetParam() );

    setTimeProviderForLoop( loop );

    TaskLoopTestHelper::Counter counter;

    const auto threadsCountWithMain = GetParam() + 1;

    auto future = runInThread( loop );

    for( size_t i = 0; i < threadsCountWithMain; ++i ) {
        loop.createPostponedTask( [&]( PostponedTask& ) {
                ++counter;

                counter.wait( threadsCountWithMain + 1 );

                loop.stop();
            } )
            ->start( TaskLoop::Interval( rand() % 6 + 1 ), false );
    }

    sleep( loop, TaskLoop::Interval( 6 ) );

    counter.wait( threadsCountWithMain );

    ++counter;
}

TEST_P( TaskLoopTestSuite, postponedZeroIntervalPeriodicTask ) {
    TaskLoop loop( GetParam() );

    TaskLoopTestHelper::Counter counter1;
    TaskLoopTestHelper::Counter counter2;

    loop.createPostponedTask( [&]( PostponedTask& task ) {
            ++counter1;
            if( counter1.get() == 10u ) {
                task.stop();

                EXPECT_EQ( getTasksCount( loop ), 0 );

                loop.createPostponedTask( [&]( PostponedTask& ) {
                        ++counter2;
                        if( counter2.get() == 5u ) {
                            loop.stop();
                        }
                    } )
                    ->start( TaskLoop::Interval( 0 ), true );
            }
        } )
        ->start( TaskLoop::Interval( 0 ), true );

    EXPECT_EQ( getTasksCount( loop ), 1u );

    loop.run();

    if( GetParam() == 0 ) {
        EXPECT_EQ( getTasksCount( loop ), 1u );  // leaved task. Is it right?
        EXPECT_EQ( counter1.get(), 10u );
        EXPECT_EQ( counter2.get(), 5u );
    }
}

TEST_P( TaskLoopTestSuite, postponedSeveralTasks ) {
    TaskLoop loop( GetParam() );

    setTimeProviderForLoop( loop );

    TaskLoopTestHelper::Counter counter0, counter, counter50, counter5, counter9, counter5_2, counter10_and_40;

    auto future = runInThread( loop );

    auto addSimpleTask = [&]() {
        loop.createPostponedTask( [&]( PostponedTask& ) { ++counter; } )->start( TaskLoop::Interval( 0 ), false );
    };

    loop.createPostponedTask( [&]( PostponedTask& ) { ++counter0; } )->start( TaskLoop::Interval( 0 ), true );

    loop.createPostponedTask( [&]( PostponedTask& ) {
            ++counter50;

            addSimpleTask();
        } )
        ->start( TaskLoop::Interval( 50 ), true );

    loop.createPostponedTask( [&]( PostponedTask& ) {
            ++counter10_and_40;

            addSimpleTask();
        } )
        ->start( TaskLoop::Interval( 10 ), TaskLoop::Interval( 40 ) );

    loop.createPostponedTask( [&]( PostponedTask& ) {
            ++counter5;

            addSimpleTask();
        } )
        ->start( TaskLoop::Interval( 5 ), true );

    loop.createPostponedTask( [&]( PostponedTask& ) {
            ++counter9;

            addSimpleTask();
        } )
        ->start( TaskLoop::Interval( 9 ), true );

    loop.createPostponedTask( [&]( PostponedTask& ) {
            ++counter5_2;

            addSimpleTask();
        } )
        ->start( TaskLoop::Interval( 5 ), true );

    // 2
    {
        sleep( loop, TaskLoop::Interval( 2 ) );

        counter5.wait( 0 );
        counter5_2.wait( 0 );
    }

    // 4
    {
        sleep( loop, TaskLoop::Interval( 2 ) );

        counter5.wait( 0 );
        counter5_2.wait( 0 );
        counter.wait( 0 );
    }

    // 5
    {
        sleep( loop, TaskLoop::Interval( 1 ) );

        counter5.wait( 1 );
        counter5_2.wait( 1 );
        counter9.wait( 0 );
        counter.wait( 2 );
    }

    // 9
    {
        sleep( loop, TaskLoop::Interval( 4 ) );

        counter5.wait( 1 );
        counter5_2.wait( 1 );
        counter9.wait( 1 );
        counter10_and_40.wait( 0 );
        counter.wait( 3 );
    }

    // 10
    {
        sleep( loop, TaskLoop::Interval( 1 ) );

        counter5.wait( 2 );
        counter5_2.wait( 2 );
        counter9.wait( 1 );
        counter10_and_40.wait( 1 );
        counter.wait( 6 );
    }

    // 49
    {
        sleep( loop, TaskLoop::Interval( 39 ) );

        counter5.wait( 9 );
        counter5_2.wait( 9 );
        counter9.wait( 5 );
        counter10_and_40.wait( 1 );
        counter50.wait( 0 );
        counter.wait( 24 );
    }

    // 50
    {
        sleep( loop, TaskLoop::Interval( 1 ) );

        counter5.wait( 10 );
        counter5_2.wait( 10 );
        counter9.wait( 5 );
        counter50.wait( 1 );
        counter10_and_40.wait( 2 );
        counter.wait( 28 );
    }

    // stop + 150
    {
        loop.stop();

        sleep( loop, TaskLoop::Interval( 100 ) );

        counter5.wait( 10 );
        counter5_2.wait( 10 );
        counter9.wait( 5 );
        counter50.wait( 1 );
        counter10_and_40.wait( 2 );
        counter.wait( 28 );
    }

    EXPECT_TRUE( counter0.get() );
}

TEST_P( TaskLoopTestSuite, restartPostponedTask ) {
    TaskLoop loop( GetParam() );

    setTimeProviderForLoop( loop );

    TaskLoopTestHelper::Counter counter0, counter50;

    auto future = runInThread( loop );

    loop.createPostponedTask( [&]( PostponedTask& ) { ++counter0; } )->start( TaskLoop::Interval( 0 ), true );

    auto task2 = loop.createPostponedTask( [&]( PostponedTask& ) { ++counter50; } );

    auto task1 = loop.createPostponedTask( [&]( PostponedTask& ) { ++counter50; } );

    for( int i = 0; i < 100; ++i ) {
        task2->start( TaskLoop::Interval( 50 ), rand() % 2 );
        task2->start( TaskLoop::Interval( 50 ), rand() % 2 );
        sleep( loop, TaskLoop::Interval( 49 ) );
    }

    counter50.wait( 0 );

    loop.stop();
}

TEST_P( TaskLoopTestSuite, onTerminationSignalRecieved ) {
    constexpr const size_t ExperimentsCount = 5;
    for( size_t experiment = 0; experiment < ExperimentsCount; ++experiment ) {
        TaskLoop loop( GetParam() );

        auto future = runInThread( loop );

        for( size_t i = 0; i < 50u; ++i ) {
            loop.addTask( []() {}, true );
        }

        std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );

        loop.stop( TaskLoop::StoppingMode::OnTerminationSignalReceived );

        EXPECT_FALSE( isLoopRunning( loop ) );
    }
}

TEST_P( TaskLoopTestSuite, changeTimeout ) {
    TaskLoop loop( GetParam() );

    setTimeProviderForLoop( loop );

    size_t counter = 0, counter2 = 0;
    size_t number = 0;

    std::mutex mutex;
    std::condition_variable condVar1;
    std::condition_variable condVar2;

    constexpr const size_t TasksCountFactor = 50u;
    constexpr const size_t Range = 20u;

    const auto threadsCount = GetParam() + 1;

    std::vector< PostponedTaskPtr > tasks;
    tasks.reserve( threadsCount * TasksCountFactor );

    auto future = runInThread( loop );

    for( size_t i = 0; i < threadsCount * TasksCountFactor; ++i ) {
        tasks.push_back( loop.createPostponedTask( [&]( PostponedTask& ) {
            std::unique_lock< std::mutex > locker( mutex );
            const auto currentIndex = ( number++ / threadsCount ) + 1;
            ++counter;

            if( counter == threadsCount * currentIndex ) {
                condVar1.notify_all();
            }

            condVar2.wait( locker, [&]() { return counter2 == currentIndex; } );
        } ) );
    }

    for( int i = 10; i >= 0; --i ) {
        for( auto task : tasks ) {
            task->start( PostponedTask::Interval( 10u * size_t( i ) + size_t( rand() ) % Range ), false );
        }
    }

    sleep( loop, TaskLoop::Interval( Range ) );

    for( size_t i = 0; i < TasksCountFactor; ++i ) {
        std::unique_lock< std::mutex > locker( mutex );

        condVar1.wait( locker, [&]() { return counter == threadsCount * ( i + 1 ); } );

        ++counter2;

        condVar2.notify_all();
    }

    loop.stop();
}

TEST_P( TaskLoopTestSuite, stoppingTaskWithWaiting ) {
    TaskLoop loop( GetParam() );

    std::atomic< bool > isRunning{ false };
    std::atomic< size_t > counter{ 0 };

    auto future = runInThread( loop );

    auto task = loop.createPostponedTask( [&]( PostponedTask& ) {
        isRunning = true;

        ++counter;
        std::this_thread::sleep_for( std::chrono::milliseconds( 4 ) );
        --counter;
    } );

    task->start( TaskLoop::Interval( 0 ), true );

    while( !isRunning ) {
        std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
    }

    task->stop();

    EXPECT_EQ( counter, 0 );
    EXPECT_EQ( getTasksCount( loop ), 0 );

    loop.stop();

    EXPECT_EQ( counter, 0 );
}

TEST_P( TaskLoopTestSuite, condVarNotificationBeforeCallback ) {
    TaskLoop loop( GetParam() + 1 );

    const auto threadsCount = GetParam() + 2;
    std::vector< TaskLoopTestHelper::Counter > counters( threadsCount );

    auto future = runInThread( loop );

    for( size_t i = 0; i < threadsCount; ++i ) {
        loop.createPostponedTask( [&]( PostponedTask& ) { FAIL(); } )->start( std::chrono::hours( 1 ), false );
    }

    for( size_t i = 0; i < threadsCount; ++i ) {
        loop.createPostponedTask( [&, i]( PostponedTask& ) {
                if( i + 1 != threadsCount ) {
                    counters[ i + 1 ].wait( 1 );
                }
                ++counters[ i ];
            } )
            ->start( std::chrono::milliseconds( 4 + i * 2 ), false );
    }

    counters[ 0 ].wait( 1 );

    loop.stop();
}

TEST_P( TaskLoopTestSuite, condVarNotificationBeforeCallback_parallelTasksAdding ) {
    TaskLoop loop( GetParam() + 1 );

    const auto threadsCount = GetParam() + 2;

    TaskLoopTestHelper::Counter frontTaskCounter;
    TaskLoopTestHelper::Counter addingTasksCounter;

    auto future = runInThread( loop );

    for( size_t i = 0; i < threadsCount - 1; ++i ) {
        loop.createPostponedTask( [&]( PostponedTask& ) { FAIL(); } )->start( std::chrono::hours( 1 ), false );
    }

    std::vector< std::future< void > > frontTasksAddingFutures;

    for( size_t i = 0; i < 2u; ++i ) {
        frontTasksAddingFutures.push_back( std::async( std::launch::async, [&]() {
            ++addingTasksCounter;
            addingTasksCounter.wait( 2u );

            loop.addTask( [&]() { ++frontTaskCounter; } );
        } ) );
    }

    frontTaskCounter.wait( 2u );

    loop.stop();
}

TEST_F( TaskLoopTestSuite, mulptipleStopsAndRestarts ) {
    TaskLoop loop( 0 );

    setTimeProviderForLoop( loop );

    constexpr const size_t TasksCount = 128u;

    std::bitset< TasksCount > tasksBitset;
    std::bitset< TasksCount > currentBitset;
    TaskLoopTestHelper::Counter counter;

    std::vector< std::pair< size_t, PostponedTaskPtr > > tasks;
    tasks.reserve( TasksCount );

    auto future = runInThread( loop );

    for( size_t i = 0; i < TasksCount; ++i ) {
        tasks.emplace_back( i + 1, nullptr );
    }

    std::random_shuffle( tasks.begin(), tasks.end() );

    for( auto& indexToTask : tasks ) {
        const auto index = indexToTask.first;
        indexToTask.second = loop.createPostponedTask( [&, index]( PostponedTask& ) {
            EXPECT_FALSE( tasksBitset[ index - 1 ] );
            tasksBitset[ index - 1 ] = true;

            EXPECT_EQ( tasksBitset, currentBitset );

            ++counter;
        } );

        indexToTask.second->start( TaskLoop::Interval( index ), false );
    }

    for( size_t i = 0; i < TasksCount; ++i ) {
        if( i % 2 ) {
            tasks[ i ].second->start( TaskLoop::Interval( tasks[ i ].first + 64u ), false );
            tasks[ i ].second->start( TaskLoop::Interval( tasks[ i ].first + 32u ), false );
        }
        else {
            tasks[ i ].second->stop();
            tasks[ i ].second = nullptr;
        }
    }

    tasks.erase( std::remove_if( tasks.begin(),
                                 tasks.end(),
                                 []( const std::pair< size_t, PostponedTaskPtr >& indexToTask ) {
                                     return indexToTask.second == nullptr;
                                 } ),
                 tasks.end() );

    std::sort( tasks.begin(), tasks.end() );

    sleep( loop, TaskLoop::Interval( 32u ) );

    size_t prev = 0;
    for( size_t i = 0; i < tasks.size(); ++i ) {
        currentBitset[ tasks[ i ].first - 1 ] = true;

        sleep( loop, TaskLoop::Interval( tasks[ i ].first - prev ) );
        prev = tasks[ i ].first;

        counter.wait( i + 1 );
    }

    loop.stop();
}

TEST_F( TaskLoopTestSuite, periodicModeFixedIdleDuration ) {
    TaskLoop loop( 0 );

    setTimeProviderForLoop( loop );

    auto future = runInThread( loop );

    TaskLoopTestHelper::Counter counter;

    auto task = loop.createPostponedTask( [&]( PostponedTask& ) {
        sleep( loop, TaskLoop::Interval( 2 ) );
        loop.addTask( [&counter]() { ++counter; } );
    } );
    task->setPeriodicMode( PostponedTask::PeriodicMode::FixedIdleDuration );
    task->start( TaskLoop::Interval( 10 ), true );

    sleep( loop, TaskLoop::Interval( 50 ) );
    counter.wait( 1 );

    sleep( loop, TaskLoop::Interval( 9 ) );
    counter.wait( 1 );

    sleep( loop, TaskLoop::Interval( 1 ) );
    counter.wait( 2 );

    loop.stop();
}

TEST_P( TaskLoopTestSuite, postTasks ) {
    TaskLoop loop( GetParam() );
    auto future = runInThread( loop );

    std::atomic_int counter{ 2 };

    auto even = [&]() {
        for( int i = 1; i < 10; ++i ) {
            // Should never be odd
            EXPECT_EQ( counter % 2, 0 );

            std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
            counter = 2 * i;
        }
    };

    auto taskDonePromise = std::promise< bool >();
    auto odd = [&]() {
        counter = 1;
        for( int i = 1; i < 10; ++i ) {
            // Should never be even
            EXPECT_NE( counter % 2, 0 );

            std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
            counter = 2 * i + 1;
        }
        taskDonePromise.set_value( true );
    };

    StrandId strandId = loop.postTask( even );
    EXPECT_NE( strandId, -1 );
    loop.postTask( strandId, odd );

    EXPECT_TRUE( taskDonePromise.get_future().get() );

    loop.stop();
}

TEST_F( TaskLoopTestSuite, taskWithCapturedObjectThatLocksInDestructor ) {
    // EFWR-3600 Deadlock in task loop
    // Task with captured object that locks external mutex in destructor
    // External thread is holding external mutex that causes lock in task loop

    TaskLoopTestHelper::Counter counter1;
    TaskLoopTestHelper::Counter counter2;

    std::mutex externalMutex;
    auto sharedObject = std::make_shared< ExecuteOnExit >( [&externalMutex, &counter1]() mutable {
        ++counter1;
        std::lock_guard< std::mutex > lock( externalMutex );
    } );
    auto task = [sharedObject]() { /*dummy task*/ };
    sharedObject.reset();  // pass ownership to task function

    std::unique_lock< std::mutex > externalLock( externalMutex );

    TaskLoop loop( 0 );
    auto future = runInThread( loop );
    loop.addTask( std::move( task ) );

    counter1.wait( 1u );
    EXPECT_EQ( counter1.get(), 1u );

    // NOTE: at this point task loop is locked by external mutex

    // Task loop mutex should be unlocked to avoid deadlock
    loop.addTask( [&counter2] { ++counter2; } );

    externalLock.unlock();

    counter2.wait( 1u );
    EXPECT_EQ( counter2.get(), 1u );

    loop.stop();
}

INSTANTIATE_TEST_CASE_P( DiffThreadsCount, TaskLoopTestSuite, ::testing::Values( 0, 1, 3, 8, 12 ) );

}  // namespace UT
}  // namespace Services
}  // namespace HubPlatform
