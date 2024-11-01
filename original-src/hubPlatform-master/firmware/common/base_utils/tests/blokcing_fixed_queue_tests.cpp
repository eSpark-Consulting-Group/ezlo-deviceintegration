#include "base_utils/blocking_fixed_queue.h"

#include <gtest/gtest.h>
#include <vector>
#include <thread>
#include <future>

namespace HubPlatform {
namespace Container {
namespace UT {

TEST( BlockingFixedQueueTestSuite, blockingQueuePush ) {
    size_t size = 1000;
    BlockingFixedQueue< int > blk{ 100 };
    for( size_t i = 1; i <= size; i++ ) {
        blk.push( i );

        if( i <= blk.getQueueLimit() ) {
            EXPECT_LE( blk.size(), blk.getQueueLimit() );
        }
        else {
            EXPECT_EQ( blk.size(), blk.getQueueLimit() );
        }
    }
}

}  // namespace UT
}  // namespace Container
}  // namespace HubPlatform
