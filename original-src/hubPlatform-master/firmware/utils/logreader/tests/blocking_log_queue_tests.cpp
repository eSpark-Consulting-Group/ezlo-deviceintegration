#include "logreader/log_utils/blocking_log_queue.h"
#include "logreader/log_utils/blocking_fixed_log_queue.h"
#include "common_utils/cpp14.h"

#include <future>
#include <gtest/gtest.h>

namespace HubPlatform {
namespace LogReader {
namespace UT {

#define QUEUE_LIMIT 5

using BlckLogQueue = Container::BlockingLogQueue;
using BlckFixedLogQueue = Container::BlockingFixedLogQueue;

class BlckLogQueueTestSuite : public testing::TestWithParam< size_t > {
public:
    explicit BlckLogQueueTestSuite() = default;

    void SetUp() override {
        m_queue = std::make_shared< BlckLogQueue >();
    }

    void forEachPush( size_t size ) {
        for( size_t i = 1; i <= size; ++i ) {
            m_queue->push( std::to_string( i ) );
        }
    }

    void forEachPop( const size_t consumerIndex, size_t count, std::function< void( const size_t ) > func ) {
        for( size_t i = 1; i <= count; i++ ) {
            std::string popValue;
            EXPECT_TRUE( m_queue->pop( consumerIndex, popValue ) );
            if( func ) {
                func( i );
            }
        }
    }

    std::vector< std::future< size_t > > multiThreadLoopPop( size_t threadCount ) {
        std::vector< std::future< size_t > > results;
        for( size_t consumerIndex = 0; consumerIndex < threadCount; consumerIndex++ ) {
            results.emplace_back(
                std::async( std::launch::async, &BlckLogQueueTestSuite::loopPop, this, consumerIndex ) );
        }
        return results;
    }

    size_t loopPop( size_t index ) {
        int count = 0;
        std::string value;
        while( m_queue->pop( index, value ) ) {
            ++count;
        }
        return count;
    }

    void syncPushQueue( const size_t threadCount, const size_t itemCount ) {
        auto threads = asyncPushQueue( threadCount, itemCount );
        joinMultThread( threads );
    }

    std::vector< std::thread > asyncPushQueue( const size_t threadCount, const size_t itemCount ) {
        std::vector< std::thread > threads;
        for( size_t i = 1; i <= threadCount; i++ )
            threads.emplace_back( [&]( const size_t count ) { forEachPush( count ); }, itemCount );

        return threads;
    }

    void joinMultThread( std::vector< std::thread >& threads ) {
        for( auto& thread : threads ) {
            if( thread.joinable() ) {
                thread.join();
            }
        }
        m_queue->unblockConsumer();
    }

    size_t getAsyncResultTotalPopCount( std::vector< std::future< size_t > >& results,
                                        std::function< void( const size_t ) > func = nullptr ) {
        size_t totalPop = 0;

        for( auto& it : results ) {
            size_t data = it.get();
            if( func ) {
                func( data );
            }
            totalPop += data;
        }
        return totalPop;
    }

protected:
    std::shared_ptr< BlckLogQueue > m_queue;
};

namespace BlckLogQueueZeroRegCons {
TEST_F( BlckLogQueueTestSuite, oneThreadPush ) {
    size_t itemSize = 100;
    forEachPush( itemSize );
    EXPECT_EQ( m_queue->size(), itemSize );
}

TEST_F( BlckLogQueueTestSuite, popSuccess ) {
    size_t itemSize = 100;
    size_t consumerIndex = 0;
    forEachPush( itemSize );
    auto func = [&]( const size_t index ) { EXPECT_EQ( m_queue->size(), itemSize - index ); };
    forEachPop( consumerIndex, itemSize, func );
}

TEST_F( BlckLogQueueTestSuite, popUnsuccess ) {
    size_t consumerIndex = 0;
    std::string popValue( "data" );
    m_queue->unblockConsumer();
    EXPECT_FALSE( m_queue->pop( consumerIndex, popValue ) );
    EXPECT_EQ( popValue, "data" );
    EXPECT_EQ( m_queue->size(), 0 );
}

TEST_F( BlckLogQueueTestSuite, clear ) {
    size_t itemSize = 100;
    forEachPush( itemSize );
    EXPECT_EQ( m_queue->size(), itemSize );
    m_queue->clear();

    uint8_t consumer = 0;
    std::string popValue( "data" );
    m_queue->unblockConsumer();
    EXPECT_FALSE( m_queue->pop( consumer, popValue ) );
    EXPECT_EQ( popValue, "data" );
    EXPECT_EQ( m_queue->size(), 0 );
}

TEST_F( BlckLogQueueTestSuite, onePushThreadAndOnePopThread ) {
    size_t size = 100000;

    syncPushQueue( 1, size );
    uint8_t consumer = 0;
    size_t result = loopPop( consumer );
    EXPECT_EQ( result, size );
}

TEST_F( BlckLogQueueTestSuite, onePushThreadsAndMultiPopThreads ) {
    size_t size = 1000;
    size_t threadCount = 100;
    size_t expectedCount = size * threadCount;

    auto threads = asyncPushQueue( threadCount, size );
    auto results = multiThreadLoopPop( threadCount );
    joinMultThread( threads );
    size_t totalPop = getAsyncResultTotalPopCount( results );
    EXPECT_EQ( totalPop, expectedCount );
}

TEST_F( BlckLogQueueTestSuite, multiPushThreadsAndOnePopThread ) {
    size_t size = 1000;
    size_t threadCount = 100;
    size_t expectedCount = size * threadCount;
    syncPushQueue( threadCount, size );
    uint8_t consumer = 0;
    size_t result = loopPop( consumer );
    EXPECT_EQ( result, expectedCount );
}

TEST_F( BlckLogQueueTestSuite, multiPushThreadsAndMultiPopThreads ) {
    size_t size = 1000;
    size_t threadCount = 100;
    size_t expectedCount = size * threadCount;

    auto threads = asyncPushQueue( threadCount, size );
    auto results = multiThreadLoopPop( threadCount );
    joinMultThread( threads );
    size_t totalPop = getAsyncResultTotalPopCount( results );

    EXPECT_EQ( totalPop, expectedCount );
}

}  // namespace BlckLogQueueZeroRegCons

class BlckLogQueueMultiRegConsTestSuite : public BlckLogQueueTestSuite {
public:
    BlckLogQueueMultiRegConsTestSuite() = default;

    void SetUp() override {
        m_queue = std::make_shared< BlckLogQueue >( 3 );
    }

    void pushAndPopTests( const size_t threadCount ) {
        size_t itemSize = 1000;

        size_t expectedPopCountOfPerThread = itemSize * threadCount;
        size_t expectedTotalPopCount = expectedPopCountOfPerThread * m_queue->m_consumerSize;
        size_t totalPop = 0;

        auto threads = asyncPushQueue( threadCount, itemSize );
        std::vector< std::future< size_t > > results = multiThreadLoopPop( m_queue->m_consumerSize );

        joinMultThread( threads );
        totalPop = getAsyncResultTotalPopCount(
            results, [&]( const size_t count ) { EXPECT_EQ( expectedPopCountOfPerThread, count ); } );

        EXPECT_EQ( totalPop, expectedTotalPopCount );
    }
};

namespace BlckLogQueueMultiRegCons {

TEST_F( BlckLogQueueMultiRegConsTestSuite, popSuccess ) {
    size_t itemSize = 100;
    forEachPush( itemSize );
    for( size_t consumerIndex = 0; consumerIndex < m_queue->m_consumerSize; consumerIndex++ ) {
        if( m_queue->m_consumerSize - 1 == consumerIndex ) {  // last customer
            forEachPop( consumerIndex, itemSize, [&]( const size_t index ) {
                EXPECT_EQ( m_queue->size(), itemSize - index );
            } );
        }
        else {
            forEachPop(
                consumerIndex, itemSize, [&]( const size_t index ) { EXPECT_EQ( m_queue->size(), itemSize ); } );
        }
    }
}

TEST_F( BlckLogQueueMultiRegConsTestSuite, popUnsuccess ) {
    std::string popValue( "data" );

    /*
     *  Registered Consumer = 0, 1, 2
     */
    m_queue->unblockConsumer();
    for( size_t consumerIndex = 0; consumerIndex < m_queue->m_consumerSize; consumerIndex++ ) {
        EXPECT_FALSE( m_queue->pop( consumerIndex, popValue ) );
        EXPECT_EQ( popValue, "data" );
        EXPECT_EQ( m_queue->size(), 0 );
    }
    m_queue->reset();

    size_t unRegisteredConsumer = 3;
    std::string newData( "newData" );
    m_queue->push( newData );
    EXPECT_FALSE( m_queue->pop( unRegisteredConsumer, popValue ) );
    EXPECT_NE( popValue, newData );
    EXPECT_EQ( m_queue->size(), 1 );
}

TEST_P( BlckLogQueueMultiRegConsTestSuite, multiPushThreadsAndThreePopThreads ) {
    pushAndPopTests( GetParam() );
}

INSTANTIATE_TEST_CASE_P( MultipleThreads, BlckLogQueueMultiRegConsTestSuite, testing::Values( 1, 2, 3, 4, 5 ) );

}  // namespace BlckLogQueueMultiRegCons

class BlckFixedLogQueueTestSuite : public BlckLogQueueTestSuite {
public:
    void SetUp() override {
        m_queue = std::make_shared< BlckFixedLogQueue >();
    }

    std::shared_ptr< BlckFixedLogQueue > getQueue() {
        return std::dynamic_pointer_cast< BlckFixedLogQueue >( m_queue );
    }

    void pushAndPopTests( const size_t pushThreadCount, const size_t popThreadCount ) {
        std::shared_ptr< BlckFixedLogQueue > queue = getQueue();
        size_t size = 1000;
        size_t expectedCount = size * pushThreadCount;
        auto threads = asyncPushQueue( pushThreadCount, size );
        auto results = multiThreadLoopPop( popThreadCount );
        joinMultThread( threads );
        size_t totalPop = getAsyncResultTotalPopCount( results );
        EXPECT_LE( totalPop, expectedCount );
        EXPECT_GE( totalPop, queue->getQueueLimit() );
    }
};

namespace BlckFixedLogQueueZeroRegCons {

TEST_F( BlckFixedLogQueueTestSuite, oneThreadPush ) {
    std::shared_ptr< BlckFixedLogQueue > queue = getQueue();
    size_t itemSize = 100 + queue->getQueueLimit();
    forEachPush( itemSize );
    EXPECT_EQ( queue->size(), queue->getQueueLimit() );
}

TEST_F( BlckFixedLogQueueTestSuite, popSuccess ) {
    size_t itemSize = 100;
    size_t consumerIndex = 0;
    forEachPush( itemSize );
    auto func = [&]( const size_t index ) { EXPECT_EQ( m_queue->size(), itemSize - index ); };
    forEachPop( consumerIndex, itemSize, func );
}

TEST_F( BlckFixedLogQueueTestSuite, popUnsuccess ) {
    size_t consumerIndex = 0;
    std::string popValue( "data" );
    m_queue->unblockConsumer();
    EXPECT_FALSE( m_queue->pop( consumerIndex, popValue ) );
    EXPECT_EQ( popValue, "data" );
    EXPECT_EQ( m_queue->size(), 0 );
}

TEST_F( BlckFixedLogQueueTestSuite, clear ) {
    size_t itemSize = 100;
    forEachPush( itemSize );
    EXPECT_EQ( m_queue->size(), itemSize );
    m_queue->clear();

    uint8_t consumer = 0;
    std::string popValue( "data" );
    m_queue->unblockConsumer();
    EXPECT_FALSE( m_queue->pop( consumer, popValue ) );
    EXPECT_EQ( popValue, "data" );
    EXPECT_EQ( m_queue->size(), 0 );
}

TEST_P( BlckFixedLogQueueTestSuite, onePushThreadAndMultiPopThreads ) {
    pushAndPopTests( 1, GetParam() );
}

TEST_P( BlckFixedLogQueueTestSuite, multiPushThreadsAndOnePopThread ) {
    pushAndPopTests( GetParam(), 1 );
}

TEST_P( BlckFixedLogQueueTestSuite, multiPushThreadsAndMultiPopThreads ) {
    pushAndPopTests( GetParam(), GetParam() );
}

INSTANTIATE_TEST_CASE_P( MultipleThreads, BlckFixedLogQueueTestSuite, testing::Values( 1, 2, 3, 4, 5 ) );

}  // namespace BlckFixedLogQueueZeroRegCons

namespace BlckFixedLogQueueMultiRegCons {
constexpr const size_t ConsumerSize = 3;

class BlckFixedLogQueueMultiRegConsTestSuite : public BlckFixedLogQueueTestSuite {
public:
    void SetUp() override {
        m_queue = std::make_shared< BlckFixedLogQueue >( QUEUE_LIMIT, ConsumerSize );
    }

    void consumerOperation( const std::set< size_t > consumers, std::function< void( const size_t ) > func ) {
        for( const auto& it : consumers ) {
            func( it );
        }
    }

    void pushAndPopTests( const size_t threadCount ) {
        size_t itemSize = 100;

        size_t expectedPopCountOfPerThread = itemSize * threadCount;
        size_t expectedTotalPopCount = expectedPopCountOfPerThread * m_queue->m_consumerSize;
        size_t totalPop = 0;

        auto threads = asyncPushQueue( threadCount, itemSize );
        std::vector< std::future< size_t > > results = multiThreadLoopPop( m_queue->m_consumerSize );
        joinMultThread( threads );

        totalPop = getAsyncResultTotalPopCount( results );

        EXPECT_LE( totalPop, expectedTotalPopCount );
        EXPECT_GE( totalPop, ConsumerSize * QUEUE_LIMIT );
    }
};

TEST_F( BlckFixedLogQueueMultiRegConsTestSuite, popSuccess ) {
    size_t itemSize = QUEUE_LIMIT;

    auto f = [&]( const size_t consumerIndex ) {
        std::string popValue;
        EXPECT_TRUE( m_queue->pop( consumerIndex, popValue ) );
    };

    forEachPush( 1 );
    consumerOperation( { 0, 1, 2 }, f );
    EXPECT_EQ( m_queue->size(), 0 );
    forEachPush( QUEUE_LIMIT );
    EXPECT_EQ( m_queue->size(), QUEUE_LIMIT );
    for( size_t count = 0; count < QUEUE_LIMIT; count++ ) {
        consumerOperation( { 1, 2 }, f );
    }
    EXPECT_EQ( m_queue->size(), QUEUE_LIMIT );
    m_queue->push( "END" );
    EXPECT_EQ( m_queue->size(), QUEUE_LIMIT );
    for( size_t count = 0; count < QUEUE_LIMIT; count++ ) {
        consumerOperation( { 0 }, f );
    }
    EXPECT_EQ( m_queue->size(), 1 );
}

TEST_F( BlckFixedLogQueueMultiRegConsTestSuite, popUnsuccess ) {
    std::string popValue( "data" );

    /*
     *  Registered Consumer = 0, 1, 2
     */
    m_queue->unblockConsumer();
    for( size_t consumerIndex = 0; consumerIndex < m_queue->m_consumerSize; consumerIndex++ ) {
        EXPECT_FALSE( m_queue->pop( consumerIndex, popValue ) );
        EXPECT_EQ( popValue, "data" );
        EXPECT_EQ( m_queue->size(), 0 );
    }

    m_queue->reset();

    size_t unRegisteredConsumer = 3;
    std::string newData( "newData" );
    m_queue->push( newData );
    EXPECT_FALSE( m_queue->pop( unRegisteredConsumer, popValue ) );
    EXPECT_NE( popValue, newData );
    EXPECT_EQ( m_queue->size(), 1 );
}

TEST_P( BlckFixedLogQueueMultiRegConsTestSuite, multiPushThreadsAndThreePopThreads ) {
    pushAndPopTests( GetParam() );
}

INSTANTIATE_TEST_CASE_P( MultipleThreads, BlckFixedLogQueueMultiRegConsTestSuite, testing::Values( 1, 2, 3, 4, 5 ) );

}  // namespace BlckFixedLogQueueMultiRegCons

}  // namespace UT
}  // namespace LogReader
}  // namespace HubPlatform
