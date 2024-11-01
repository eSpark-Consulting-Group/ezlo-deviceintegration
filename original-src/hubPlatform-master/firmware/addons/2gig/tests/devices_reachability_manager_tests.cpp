#include "test_utils/test_utils.h"

#include "2gig/2gig_devices_reachability_manager.h"
#include "2gig/tests/fakes/in_memory_persistence_client.h"

#include "fakes/fake_devices_repository.h"
#include "mocks/2gig_client_mock.h"

#include "common_utils/cpp14.h"

#include <gtest/gtest.h>
#include <thread>
#include <chrono>

/*

TESTPLAN

DONE    1. Mark as unreachable initiated nodes
DONE    2. Mark as reachable after initiation
DONE    3. Mark as reachable after unreachable
DONE    4. Do not mark as reachable repeatedly
DONE    5. Mark part of devices unreachable
DONE    6. React on device adding
DONE    6. React on device removing

*/

namespace HubPlatform {
namespace TwoGig {
namespace UT {

using namespace ::testing;

struct DevicesReachabilityManagerTestSuite : public testing::Test {
    DevicesReachabilityManagerTestSuite() {}

    void SetUp() override {
        takeDevices().reset();
        m_devicesReachabilityManager.reset();
    }

    void init( std::chrono::system_clock::duration unreachableInterval = std::chrono::milliseconds{ 100 } ) {
        m_devicesReachabilityManager =
            std::make_unique< DevicesReachabilityManagerImpl >( m_client, m_devices, unreachableInterval );
    }

    FakeDevicesRepository& takeDevices() {
        return m_devices;
    }

    DevicesReachabilityManagerImpl& takeReachabilityManager() {
        EXPECT_TRUE( m_devicesReachabilityManager.get() != nullptr );
        return *m_devicesReachabilityManager;
    }

    void waitInterval( std::chrono::system_clock::duration interval = std::chrono::milliseconds{ 150 } ) {
        std::this_thread::sleep_for( interval );
    }

protected:
    ClientMock m_client;
    FakeDevicesRepository m_devices;
    std::unique_ptr< DevicesReachabilityManagerImpl > m_devicesReachabilityManager;
};

TEST_F( DevicesReachabilityManagerTestSuite, markUnreachableInitiated ) {
    takeDevices().addDevice( { 1 } );
    takeDevices().addDevice( { 2, 3 } );
    takeDevices().addDevice( { 4 } );

    init();
    waitInterval();

    EXPECT_CALL( m_client, onDeviceReachableChanged( 1, false ) ).Times( Exactly( 1 ) );
    EXPECT_CALL( m_client, onDeviceReachableChanged( 2, false ) ).Times( Exactly( 1 ) );
    EXPECT_CALL( m_client, onDeviceReachableChanged( 3, false ) ).Times( Exactly( 1 ) );
    EXPECT_CALL( m_client, onDeviceReachableChanged( 4, false ) ).Times( Exactly( 1 ) );

    takeReachabilityManager().checkAllDevicesReachableStates();
}

TEST_F( DevicesReachabilityManagerTestSuite, markReachableAfterInitiation ) {
    takeDevices().addDevice( { 1 } );

    init();

    EXPECT_CALL( m_client, onDeviceReachableChanged( 1, true ) ).Times( Exactly( 1 ) );

    takeReachabilityManager().onDeviceReceivedData( 1 );
}

TEST_F( DevicesReachabilityManagerTestSuite, markReachableAfterUnreachable ) {
    takeDevices().addDevice( { 1 } );

    init();

    EXPECT_CALL( m_client, onDeviceReachableChanged( 1, true ) ).Times( Exactly( 1 ) );
    takeReachabilityManager().onDeviceReceivedData( 1 );
    waitInterval();

    EXPECT_CALL( m_client, onDeviceReachableChanged( 1, false ) ).Times( Exactly( 1 ) );
    takeReachabilityManager().checkAllDevicesReachableStates();

    EXPECT_CALL( m_client, onDeviceReachableChanged( 1, true ) ).Times( Exactly( 1 ) );

    takeReachabilityManager().onDeviceReceivedData( 1 );
}

TEST_F( DevicesReachabilityManagerTestSuite, doNotMarkReachableRepeatedly ) {
    takeDevices().addDevice( { 1 } );

    init();

    EXPECT_CALL( m_client, onDeviceReachableChanged( 1, true ) ).Times( Exactly( 1 ) );
    takeReachabilityManager().onDeviceReceivedData( 1 );

    EXPECT_CALL( m_client, onDeviceReachableChanged( 1, true ) ).Times( Exactly( 0 ) );
    takeReachabilityManager().onDeviceReceivedData( 1 );
}

TEST_F( DevicesReachabilityManagerTestSuite, markPartOfDevicesUnreachable ) {
    takeDevices().addDevice( { 1 } );
    takeDevices().addDevice( { 2, 3 } );
    takeDevices().addDevice( { 4 } );

    init();
    waitInterval( std::chrono::milliseconds{ 50 } );

    takeReachabilityManager().onDeviceReceivedData( 1 );
    takeReachabilityManager().onDeviceReceivedData( 3 );

    waitInterval( std::chrono::milliseconds{ 70 } );

    EXPECT_CALL( m_client, onDeviceReachableChanged( 1, false ) ).Times( Exactly( 0 ) );
    EXPECT_CALL( m_client, onDeviceReachableChanged( 2, false ) ).Times( Exactly( 1 ) );
    EXPECT_CALL( m_client, onDeviceReachableChanged( 3, false ) ).Times( Exactly( 0 ) );
    EXPECT_CALL( m_client, onDeviceReachableChanged( 4, false ) ).Times( Exactly( 1 ) );

    takeReachabilityManager().checkAllDevicesReachableStates();
}

TEST_F( DevicesReachabilityManagerTestSuite, reactOnDeviceAdding ) {
    takeDevices().addDevice( { 1 } );
    takeDevices().addDevice( { 2, 3 } );

    init();
    waitInterval();

    EXPECT_CALL( m_client, onDeviceReachableChanged( 1, false ) ).Times( Exactly( 1 ) );
    EXPECT_CALL( m_client, onDeviceReachableChanged( 2, false ) ).Times( Exactly( 1 ) );
    EXPECT_CALL( m_client, onDeviceReachableChanged( 3, false ) ).Times( Exactly( 1 ) );
    EXPECT_CALL( m_client, onDeviceReachableChanged( 4, false ) ).Times( Exactly( 0 ) );

    takeReachabilityManager().checkAllDevicesReachableStates();

    EXPECT_CALL( m_client, onDeviceReachableChanged( 1, false ) ).Times( Exactly( 1 ) );
    EXPECT_CALL( m_client, onDeviceReachableChanged( 2, false ) ).Times( Exactly( 1 ) );
    EXPECT_CALL( m_client, onDeviceReachableChanged( 3, false ) ).Times( Exactly( 1 ) );
    EXPECT_CALL( m_client, onDeviceReachableChanged( 4, false ) ).Times( Exactly( 1 ) );

    takeReachabilityManager().onDeviceAdded( { 4 } );
    waitInterval();
    takeReachabilityManager().checkAllDevicesReachableStates();
}

TEST_F( DevicesReachabilityManagerTestSuite, reactOnDeviceRemove ) {
    takeDevices().addDevice( { 1 } );
    takeDevices().addDevice( { 2, 3 } );
    takeDevices().addDevice( { 4 } );

    init();
    waitInterval();

    EXPECT_CALL( m_client, onDeviceReachableChanged( 1, false ) ).Times( Exactly( 1 ) );
    EXPECT_CALL( m_client, onDeviceReachableChanged( 2, false ) ).Times( Exactly( 1 ) );
    EXPECT_CALL( m_client, onDeviceReachableChanged( 3, false ) ).Times( Exactly( 1 ) );
    EXPECT_CALL( m_client, onDeviceReachableChanged( 4, false ) ).Times( Exactly( 1 ) );

    takeReachabilityManager().checkAllDevicesReachableStates();

    EXPECT_CALL( m_client, onDeviceReachableChanged( 1, false ) ).Times( Exactly( 1 ) );
    EXPECT_CALL( m_client, onDeviceReachableChanged( 2, false ) ).Times( Exactly( 1 ) );
    EXPECT_CALL( m_client, onDeviceReachableChanged( 3, false ) ).Times( Exactly( 1 ) );
    EXPECT_CALL( m_client, onDeviceReachableChanged( 4, false ) ).Times( Exactly( 0 ) );

    takeReachabilityManager().onDeviceRemoved( { 4 } );
    waitInterval();
    takeReachabilityManager().checkAllDevicesReachableStates();
}

}  // namespace UT
}  // namespace TwoGig
}  // namespace HubPlatform
