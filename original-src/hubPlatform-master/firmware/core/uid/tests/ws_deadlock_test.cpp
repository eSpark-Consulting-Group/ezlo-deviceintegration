#include "uid/nma_client.h"
#include <gtest/gtest.h>

namespace HubPlatform {
namespace CustomNodes {
namespace UT {

class WsDeadlockTest : public ::testing::Test {
public:
};

TEST_F( WsDeadlockTest, globalLockFromLibtinyUsageWithSmartPointers ) {
    // Network object uses traits class to set mutex from to lock smart pointer operations
    Uid::NmaClientPtr networkObjectPtr;
    auto& pointerMutex = networkObjectPtr.ref_mtx();

    // libtiny library global mutex
    auto& libTinyMutex = tiny::get_ref_mtx();

    ASSERT_NE( &libTinyMutex, &pointerMutex ) << "Should use different mutexes";
}

}  // namespace UT
}  // namespace CustomNodes
}  // namespace HubPlatform