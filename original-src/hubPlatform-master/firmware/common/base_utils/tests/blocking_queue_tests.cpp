#include "base_utils/blocking_queue.h"

#include <gtest/gtest.h>
#include <vector>
#include <thread>
#include <future>

namespace HubPlatform {
namespace Container {
namespace UT {

class BlockingQueueTestSuite : public ::testing::Test {
public:
    void forEachPush( size_t size ) {
        for( size_t i = 1; i <= size; ++i ) {
            blkQueue.push( i );
        }
    }

    void forEachPop( size_t count, std::function< void( int, int ) > func ) {
        for( size_t i = 1; i <= count; i++ )
            func( i, count - i );
    }

    size_t loopPop() {
        int count = 0;
        int value;
        while( blkQueue.pop( value ) ) {
            ++count;
        }
        return count;
    }

    void asyncPushQueue( size_t threadCount, size_t itemCount ) {
        std::vector< std::thread > threads;
        for( size_t i = 1; i <= threadCount; i++ )
            threads.push_back( std::thread( [&]() { this->forEachPush( itemCount ); } ) );

        for( auto&& i : threads ) {
            if( i.joinable() )
                i.join();
        }
        blkQueue.unblockConsumer();
    }

protected:
    BlockingQueue< int > blkQueue;
};

TEST_F( BlockingQueueTestSuite, blockingQueuePush ) {
    size_t size = 100;
    forEachPush( size );
    EXPECT_EQ( blkQueue.size(), size );
}

TEST_F( BlockingQueueTestSuite, blockingQueuePopSuccess ) {
    size_t size = 100;
    forEachPush( size );
    auto f = [this]( int expectedValue, int expectedCount ) {
        int popedValue;
        EXPECT_TRUE( blkQueue.pop( popedValue ) );
        EXPECT_EQ( popedValue, expectedValue );
        EXPECT_EQ( blkQueue.size(), expectedCount );
    };
    forEachPop( size, f );
}

TEST_F( BlockingQueueTestSuite, blockingQueuePopUnsuccess ) {
    blkQueue.unblockConsumer();
    int popedValue = -999;
    EXPECT_FALSE( blkQueue.pop( popedValue ) );
    EXPECT_EQ( popedValue, -999 );
    EXPECT_EQ( blkQueue.size(), 0 );
}

TEST_F( BlockingQueueTestSuite, blockingQueueClear ) {
    size_t size = 100;
    forEachPush( size );
    EXPECT_EQ( blkQueue.size(), size );
    blkQueue.clear();

    int popedValue = -999;
    blkQueue.unblockConsumer();
    EXPECT_FALSE( blkQueue.pop( popedValue ) );
    EXPECT_EQ( popedValue, -999 );
    EXPECT_EQ( blkQueue.size(), 0 );
}

TEST_F( BlockingQueueTestSuite, oneThreadPush ) {
    size_t size = 100000;
    asyncPushQueue( 1, size );
    size_t result = loopPop();
    EXPECT_EQ( result, size );
}

TEST_F( BlockingQueueTestSuite, multiThreadPush ) {
    size_t size = 1000;
    size_t threadCount = 100;
    size_t expectedCount = size * threadCount;
    asyncPushQueue( threadCount, size );
    size_t result = loopPop();
    EXPECT_EQ( result, expectedCount );
}

}  // namespace UT
}  // namespace Container
}  // namespace HubPlatform
