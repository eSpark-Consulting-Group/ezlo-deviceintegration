#pragma once

#include "service_interfaces/task_loop.h"
#include <future>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace HubPlatform {
namespace Services {
namespace UT {

class TaskLoopTestHelper {
public:
    using TimePoint = TaskLoop::TimePoint;
    using Interval = TaskLoop::Interval;

    class Counter {
    public:
        size_t get() const noexcept {
            return m_counter;
        }

        void wait( size_t value ) {
            std::unique_lock< std::mutex > locker( m_mutex );

            m_condVar.wait( locker, [&]() { return m_counter >= value; } );

            EXPECT_EQ( m_counter, value );
        }

        Counter& operator++() {
            {
                std::unique_lock< std::mutex > locker( m_mutex );
                ++m_counter;
            }
            m_condVar.notify_all();

            return *this;
        }

    private:
        size_t m_counter = 0;
        std::mutex m_mutex;
        std::condition_variable m_condVar;
    };

    class TimeProvider : public TaskLoop::TimeProvider {
    public:
        TimePoint now() const final {
            return TimePoint( TimePoint::duration( m_now.load() ) );
        }
        std::cv_status waitUntil( std::unique_lock< std::mutex >& locker,
                                  std::condition_variable& condVar,
                                  const TimePoint& timePoint ) final {
            // don't use predicate here! Spurious awaking is ok here!
            if( now() < timePoint ) {
                condVar.wait( locker );
            }

            return now() >= timePoint ? std::cv_status::timeout : std::cv_status::no_timeout;
        }

        void sleep( TaskLoop& loop, Interval interval ) {
            {
                std::unique_lock< std::mutex > tasksLocker( loop.m_mutex );
                m_now += interval.count();
            }

            loop.notifyAll();

            std::this_thread::sleep_for( std::chrono::milliseconds( 4 ) );  // wait some real time for tasks processing
        }

    private:
        std::atomic< TimePoint::duration::rep > m_now{ 0 };
    };

    class ExecuteOnExit {
    public:
        using Action = std::function< void() >;
        ExecuteOnExit( Action action )
        : m_action( std::move( action ) ) {}
        ~ExecuteOnExit() {
            if( m_action )
                m_action();
        }

    private:
        Action m_action;
    };

    std::future< void > runInThread( TaskLoop& loop ) {
        Counter startingCounter;
        auto future = std::async( std::launch::async, [&]() {
            loop.addTask( [&]() { ++startingCounter; } );
            loop.run();
        } );

        startingCounter.wait( 1 );

        return future;
    }

    void setTimeProviderForLoop( TaskLoop& loop ) {
        loop.m_timeProvider = m_timeProvider;
    }

    static TimePoint getTimeFromProvider( TaskLoop& loop ) {
        return loop.m_timeProvider.get().now();
    }

    void sleep( TaskLoop& loop, Interval interval ) {
        m_timeProvider.sleep( loop, interval );
    }

    void waitForTasks( TaskLoop& loop ) {
        Counter counter;
        loop.addTask( [&]() { ++counter; } );
        counter.wait( 1 );
    }

    size_t getTasksCount( TaskLoop& loop ) const {
        std::unique_lock< std::mutex > locker( loop.m_mutex );
        return loop.m_tasks.size();
    }

    void clearLoop( TaskLoop& loop ) {
        std::unique_lock< std::mutex > locker( loop.m_mutex );
        loop.m_tasks.clear();
    }

    bool isLoopRunning( TaskLoop& loop ) const {
        return loop.m_isRunning;
    }

private:
    TimeProvider m_timeProvider;
};

}  // namespace UT
}  // namespace Services
}  // namespace HubPlatform
