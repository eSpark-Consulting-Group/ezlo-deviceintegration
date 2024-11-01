#include "watcher/issues_monitor/handlers/issue_handler_interface.h"
#include "watcher/issues_monitor/issues_monitor.h"

#include "log_utils/ezlo_log.h"
#include "common_utils/cpp14.h"
#include "json_utils/json_utils.h"
#include <gtest/gtest.h>

#include <future>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <thread>

/*

TESTPLAN

DONE    1. Issues monitor tests
DONE        1.1 Multiple handlers

*/

namespace HubPlatform {
namespace UT {

class IssuesMonitorTestSuite : public ::testing::Test {
public:
    class FakeGenericIssueHandler : public Watcher::IssueHandlers::IssueHandlerInterface {
    public:
        void run() override {
            m_action();
        }
        std::function< void() > m_action;
    };
};

TEST_F( IssuesMonitorTestSuite, multipleHandlers ) /* 1.1 */ {
    using TimePoint = std::chrono::steady_clock::time_point;

    std::atomic_int finishedTasks{ 0 };
    std::mutex finishedTasksMutex;
    std::condition_variable finishedTasksCondition;

    std::map< std::string, std::vector< TimePoint > > timestamps;
    timestamps[ "A" ] = {};
    timestamps[ "B" ] = {};

    auto handlerA = std::make_unique< FakeGenericIssueHandler >();
    auto handlerB = std::make_unique< FakeGenericIssueHandler >();

    handlerA->m_action = [&timestamps, &finishedTasksCondition, &finishedTasks]() {
        timestamps[ "A" ].emplace_back( std::chrono::steady_clock::now() );
        if( timestamps[ "A" ].size() == 5 ) {
            ++finishedTasks;
            finishedTasksCondition.notify_one();
        }
    };

    handlerB->m_action = [&timestamps, &finishedTasksCondition, &finishedTasks]() {
        timestamps[ "B" ].emplace_back( std::chrono::steady_clock::now() );
        if( timestamps[ "B" ].size() == 5 ) {
            ++finishedTasks;
            finishedTasksCondition.notify_one();
        }
    };

    const std::chrono::milliseconds checkIntervalA{ 5 };
    const std::chrono::milliseconds checkIntervalB{ 10 };

    Watcher::IssuesMonitor monitor;
    monitor.addHandler( std::move( handlerA ), checkIntervalA );
    monitor.addHandler( std::move( handlerB ), checkIntervalB );

    std::thread monitorThread( [&monitor]() { monitor.run(); } );

    std::unique_lock< std::mutex > lock( finishedTasksMutex );
    auto result = finishedTasksCondition.wait_for(
        lock, std::chrono::seconds( 5 ), [&finishedTasks]() { return finishedTasks == 2; } );
    EXPECT_TRUE( result );

    monitor.stop();
    if( monitorThread.joinable() ) {
        monitorThread.join();
    }

    EXPECT_EQ( finishedTasks, 2 );

    auto findMinDuration = []( const std::vector< TimePoint >& timePoints ) {
        std::chrono::milliseconds minInterval{ 9999 };
        for( size_t i = 1; i < timePoints.size(); ++i ) {
            auto interval =
                std::chrono::duration_cast< std::chrono::milliseconds >( timePoints[ i ] - timePoints[ i - 1 ] );
            if( interval < minInterval )
                minInterval = interval;
        }
        return minInterval;
    };

    auto dumpTimestamps = []( const std::vector< TimePoint >& timePoints ) {
        std::stringstream ss;
        for( auto& t : timePoints ) {
            ss << std::chrono::duration_cast< std::chrono::milliseconds >( t.time_since_epoch() ).count() << ", ";
        }
        return ss.str();
    };

    ASSERT_GT( timestamps[ "A" ].size(), 1 );
    ASSERT_GT( timestamps[ "B" ].size(), 1 );

    const auto minDurationA = findMinDuration( timestamps[ "A" ] );
    const auto minDurationB = findMinDuration( timestamps[ "B" ] );

    EXPECT_GT( minDurationA.count(), 0 ) << "timestamps: " << dumpTimestamps( timestamps[ "A" ] );
    EXPECT_LT( minDurationA.count(), 2 * checkIntervalA.count() );

    EXPECT_GT( minDurationB.count(), 0 ) << "timestamps: " << dumpTimestamps( timestamps[ "B" ] );
    EXPECT_LT( minDurationB.count(), 2 * checkIntervalB.count() );
}

}  // namespace UT
}  // namespace HubPlatform
