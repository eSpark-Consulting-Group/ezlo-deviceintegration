#include "base_utils/prioritized_task_pool.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace HubPlatform {
namespace UT {

class WatcherBase {
public:
    virtual void watch( int priority ) = 0;
};

class Counter {
public:
    Counter( int countTo )
    : m_counter( 0 )
    , m_countTo( countTo )
    , isFakeWakeup( true ) {}
    Counter( const Counter& ) = delete;
    Counter( Counter&& ) = delete;
    Counter operator=( const Counter& ) = delete;
    Counter operator=( Counter&& ) = delete;

    void increment() {
        std::lock_guard< std::mutex > lock( m_mutex );
        ++m_counter;
        if( m_counter = m_countTo ) {
            isFakeWakeup = false;
            m_condVar.notify_all();
        }
    }
    int wait_for( std::chrono::milliseconds timeoutMs ) {
        std::unique_lock< std::mutex > lock( m_mutex );
        m_condVar.wait_for( lock, timeoutMs, [this]() { return !isFakeWakeup; } );
        isFakeWakeup = true;
        return m_counter;
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_condVar;
    int m_counter;
    int m_countTo;
    bool isFakeWakeup;
};

class WatchCounter : public WatcherBase {
public:
    WatchCounter( int countTo )
    : m_counter( countTo ) {}
    void watch( int priority ) override {
        m_counter.increment();
        std::lock_guard< std::mutex > lock( m_mutex );
        m_prioritySequance.push_back( priority );
    }
    int wait_for( std::chrono::milliseconds timeoutMs ) {
        return m_counter.wait_for( timeoutMs );
    }
    std::vector< int > getPrioritySequance() {
        std::lock_guard< std::mutex > lock( m_mutex );
        return m_prioritySequance;
    }

private:
    Counter m_counter;
    std::vector< int > m_prioritySequance;
    std::mutex m_mutex;
};

class Task {
public:
    Task( int priority, WatcherBase* counter )
    : m_isRunning( true )
    , m_priority( priority )
    , m_counter( counter ) {}
    Task( const Task& obj )
    : m_isRunning( obj.m_isRunning.load() )
    , m_priority( obj.m_priority )
    , m_counter( obj.m_counter ) {}
    Task( Task&& obj )
    : m_isRunning( std::move( obj.m_isRunning.load() ) )
    , m_priority( obj.m_priority )
    , m_counter( obj.m_counter ) {}
    Task& operator=( const Task& obj ) {
        m_isRunning = obj.m_isRunning.load();
        m_priority = obj.m_priority;
        m_counter = obj.m_counter;
        return *this;
    }
    void stop() {
        m_isRunning = false;
    }
    virtual void operator()() {
        m_isRunning = false;
        m_counter->watch( m_priority );
    }
    bool operator>( const Task& a ) const {
        return m_priority > a.m_priority;
    }
    bool isRunning() const {
        return m_isRunning.load();
    }

protected:
    std::atomic_bool m_isRunning;
    int m_priority;
    WatcherBase* m_counter;
};

class EndlessTask : public Task {
public:
    EndlessTask( int priority, WatcherBase* counter )
    : Task( priority, counter ) {}
    void operator()() override {
        while( m_isRunning )
            ;
        m_counter->watch( m_priority );
    }
};

class PrioritizedTaskTestSuite : public ::testing::Test {
public:
    common::PrioritizedTaskPool< Task > m_taskPool;
};

const std::chrono::milliseconds WaitTimeout = std::chrono::milliseconds( 100 );

TEST_F( PrioritizedTaskTestSuite, singleTask ) {
    const int countTo = 1;
    WatchCounter counter( countTo );
    m_taskPool.submit( Task( 0, &counter ) );
    EXPECT_EQ( counter.wait_for( WaitTimeout ), countTo );
    m_taskPool.shutdown();
}

TEST_F( PrioritizedTaskTestSuite, multipleTask ) {
    const int countTo = 4;
    WatchCounter counter( countTo );
    m_taskPool.submit( Task( 0, &counter ) );
    m_taskPool.submit( Task( 0, &counter ) );
    m_taskPool.submit( Task( 0, &counter ) );
    m_taskPool.submit( Task( 0, &counter ) );
    EXPECT_EQ( counter.wait_for( WaitTimeout ), countTo );
    m_taskPool.shutdown();
}

TEST_F( PrioritizedTaskTestSuite, checkPriority_forwardEnter ) {
    const int countTo = 5;
    WatchCounter counter( countTo );
    m_taskPool.submit( Task( 0, &counter ) );
    m_taskPool.submit( Task( 1, &counter ) );
    m_taskPool.submit( Task( 2, &counter ) );
    m_taskPool.submit( Task( 3, &counter ) );
    m_taskPool.submit( Task( 4, &counter ) );
    EXPECT_EQ( counter.wait_for( WaitTimeout ), countTo );
    std::vector< int > prioritySequance = counter.getPrioritySequance();
    for( std::size_t i = 0; i < prioritySequance.size(); ++i ) {
        EXPECT_EQ( prioritySequance[ i ], i );
    }
    m_taskPool.shutdown();
}

TEST_F( PrioritizedTaskTestSuite, checkPriority_backwardEnter ) {
    const int countTo = 2;
    WatchCounter counter( countTo );
    m_taskPool.submit( EndlessTask( 1, &counter ) );
    std::this_thread::sleep_for( WaitTimeout );
    m_taskPool.submit( Task( 0, &counter ) );
    EXPECT_EQ( counter.wait_for( WaitTimeout ), countTo );
    m_taskPool.shutdown();
}

TEST_F( PrioritizedTaskTestSuite, stopTask ) {
    const int countTo = 1;
    WatchCounter counter( countTo );
    m_taskPool.submit( EndlessTask( 1, &counter ) );
    std::this_thread::sleep_for( WaitTimeout );
    m_taskPool.stopTask();
    EXPECT_EQ( counter.wait_for( WaitTimeout ), countTo );
    m_taskPool.shutdown();
}

TEST_F( PrioritizedTaskTestSuite, setDefaultTaskAfterSubmit ) {
    const int countTo = 4;
    WatchCounter counter( countTo );
    m_taskPool.submit( EndlessTask( 1, &counter ) );
    m_taskPool.setDefaultTask( EndlessTask( 1, &counter ) );
    std::this_thread::sleep_for( WaitTimeout );
    m_taskPool.submit( EndlessTask( 1, &counter ) );
    EXPECT_EQ( counter.wait_for( WaitTimeout ), countTo );
    m_taskPool.shutdown();
}

TEST_F( PrioritizedTaskTestSuite, defaultTask ) {
    const int countTo = 1;
    WatchCounter counter( countTo );
    m_taskPool.setDefaultTask( EndlessTask( 1, &counter ) );
    EXPECT_EQ( counter.wait_for( WaitTimeout ), countTo );
    m_taskPool.shutdown();
}

}  // namespace UT
}  // namespace HubPlatform
