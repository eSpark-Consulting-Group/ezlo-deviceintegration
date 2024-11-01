#include "logreader/log_manager.h"
#include "logreader/log_writer/log_writer_interface.h"
#include "common_utils/cpp14.h"

#include <gtest/gtest.h>
#include <mutex>

namespace HubPlatform {
namespace LogReader {

namespace UT {

#define QUEUE_LIMIT 10

using LogManagerPtr = std::shared_ptr< LogManager >;
using Lock = std::mutex;
using LockGuard = std::lock_guard< Lock >;
using UniqueLock = std::unique_lock< Lock >;

class TestLogWriter : public LogWriterInterface {
public:
    TestLogWriter( size_t latency = 0 )
    : m_latency( latency ) {}

    void sendLog( const std::string& ) override {
        ++m_logCount;
        std::this_thread::sleep_for( std::chrono::microseconds( m_latency ) );
    }

    size_t getLogCount() const {
        return m_logCount;
    }

private:
    std::atomic_size_t m_logCount{ 0 };
    size_t m_latency;
};

class LogManagerTestSuite : public testing::Test {
public:
    LogManagerTestSuite() = default;

    void SetUp( std::vector< std::shared_ptr< TestLogWriter > >&& logWriters ) {
        m_logWriters = std::make_unique< std::vector< LogWriterInterfacePtr > >();
        for( const auto& it : logWriters ) {
            m_logWriters->push_back( it );
        }
        m_logManagerPtr = std::make_shared< LogManager >( *m_logWriters, QUEUE_LIMIT );
    }

    void forEachPushData( size_t count ) {
        for( size_t index = 0; index < count; index++ ) {
            m_logManagerPtr->produceLog( std::to_string( index ) );
        }
    }

    void forEachConsumer( std::function< void( size_t, std::shared_ptr< TestLogWriter > ) > callback ) {
        for( size_t index = 0; index < m_logWriters->size(); index++ ) {
            std::shared_ptr< TestLogWriter > consumer =
                std::dynamic_pointer_cast< TestLogWriter >( ( *m_logWriters )[ index ] );
            callback( index, consumer );
        }
    }

    void waitUntilQueueIsEmpty( std::chrono::seconds timeout ) {
        m_logManagerPtr->stop();
        bool finished = false;
        std::thread thr( [&] {
            m_logManagerPtr->m_logQueue.waitUntilQueueIsEmpty();

            LockGuard lock( m_waitMutex );
            finished = true;
            m_waitConditionVariable.notify_one();
        } );

        UniqueLock lock( m_waitMutex );
        auto result = m_waitConditionVariable.wait_for( lock, timeout, [&] { return finished; } );

        if( !result ) {
            m_logManagerPtr->m_logQueue.unblockWaitUntilQueueIsEmpty();
        }

        if( thr.joinable() ) {
            thr.join();
        }
    }

    ~LogManagerTestSuite() {
        m_waitConditionVariable.notify_all();
    }

private:
    mutable std::mutex m_waitMutex;
    std::condition_variable m_waitConditionVariable;
    LogManagerPtr m_logManagerPtr;
    std::unique_ptr< std::vector< LogWriterInterfacePtr > > m_logWriters;
};

TEST_F( LogManagerTestSuite, successfulDataConsumptionWithLimitedData ) {
    size_t logCount = QUEUE_LIMIT;
    size_t consumerLatency = 100;
    SetUp( { std::make_shared< TestLogWriter >( consumerLatency ),
             std::make_shared< TestLogWriter >( consumerLatency ),
             std::make_shared< TestLogWriter >( consumerLatency ) } );
    forEachPushData( logCount );
    waitUntilQueueIsEmpty( std::chrono::seconds( 5 ) );
    forEachConsumer( [&]( size_t /*index*/, std::shared_ptr< TestLogWriter > logWriter ) {
        EXPECT_EQ( logCount, logWriter->getLogCount() );
    } );
}

TEST_F( LogManagerTestSuite, unsuccessfulDataConsumptionWithFastProducer ) {
    size_t logCount = 5 * QUEUE_LIMIT;
    size_t consumerLatency = 100;

    std::function< void( size_t, std::shared_ptr< TestLogWriter > ) > testFunc =
        [&]( size_t index, std::shared_ptr< TestLogWriter > logWriter ) {
            auto count = logWriter->getLogCount();
            EXPECT_LE( count, logCount );
            EXPECT_GE( count, QUEUE_LIMIT );
        };

    SetUp( { std::make_shared< TestLogWriter >(),
             std::make_shared< TestLogWriter >(),
             std::make_shared< TestLogWriter >( consumerLatency ) } );
    forEachPushData( logCount );
    waitUntilQueueIsEmpty( std::chrono::seconds( 5 ) );
    forEachConsumer( testFunc );

    SetUp( { std::make_shared< TestLogWriter >(),
             std::make_shared< TestLogWriter >( consumerLatency ),
             std::make_shared< TestLogWriter >( consumerLatency ) } );
    forEachPushData( logCount );
    waitUntilQueueIsEmpty( std::chrono::seconds( 5 ) );
    forEachConsumer( testFunc );

    SetUp( { std::make_shared< TestLogWriter >( consumerLatency ),
             std::make_shared< TestLogWriter >( consumerLatency ),
             std::make_shared< TestLogWriter >( consumerLatency ) } );
    forEachPushData( logCount );
    waitUntilQueueIsEmpty( std::chrono::seconds( 5 ) );
    forEachConsumer( testFunc );
}

}  // namespace UT
}  // namespace LogReader
}  // namespace HubPlatform
