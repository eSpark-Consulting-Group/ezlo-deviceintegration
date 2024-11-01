#include "core/packets/packets.h"
#include "core/pipeline/reachability_handler.h"

#include "fixtures/pipe_test.h"
#include "mocks/devices_reachability_manager_mock.h"

namespace TwoGig {
namespace Test {

/*

Test plan

Done    1. Notify manager

*/

struct DeviceReachabilityHandlerTest : public PipeTest< ReachabilityHandler > {
    DeviceReachabilityHandlerTest()
    : PipeTest< ReachabilityHandler >( m_reachabilityManager ) {}

protected:
    DevicesReachabilityManagerMock m_reachabilityManager;
};

TEST_F( DeviceReachabilityHandlerTest, notifyManagerTest ) {
    Transmission packet = {};
    packet.serial = 1;

    // Initial
    ASSERT_EQ( pullPayload(), nullptr );
    EXPECT_CALL( m_reachabilityManager, onDeviceReceivedData( 1 ) ).Times( testing::Exactly( 1 ) );

    pushPayload( packet );
    ASSERT_NE( pullPayload(), nullptr );
}

}  // namespace Test
}  // namespace TwoGig
