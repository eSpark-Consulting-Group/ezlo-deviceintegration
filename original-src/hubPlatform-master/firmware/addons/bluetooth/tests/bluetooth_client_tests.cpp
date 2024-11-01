#include "bluetooth/bluetooth_addon.h"
#include "bluetooth/bluetooth_client.h"
#include "bluetooth/tests/bluetooth_mocks.h"

#include "bus/base_bus.h"
#include "service_interfaces/bus_wrapper.h"
#include "service_interfaces/tests/task_loop_test_helper.h"
#include "addon_interfaces/addon.h"
#include "common_utils/cpp14.h"
#include "test_utils/test_utils.h"
#include "database_client/fake_db/fake_db_client.h"

#include "jsonrpc/jsonrpc.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <sp.h>

namespace HubPlatform {
namespace UT {

using namespace ::testing;

class TestBus final : public BaseBus {
public:
    TestBus( const std::string& privateName, const RxCallBack& rxCallBack, bool silent, int connectionAttempts )
    : BaseBus( privateName, rxCallBack ) {}

    void start() override {}

    void stop() override {}

    void join( const char* /*group*/ ) override {}

    bool multicast( const char* group, int msgClass, const std::string& msg ) override {
        bus_rx_data_t data;
        memset( &data, 0, sizeof( bus_rx_data_t ) );
        strcpy( data.sender, getPrivateName().c_str() );
        strcpy( data.msg, msg.c_str() );
        data.msg_size = msg.size();
        data.num_groups = 1;
        strcpy( data.target_groups[ 0 ], group );
        data.msg_type = msgClass;
        data.service_type = REGULAR_MESS;
        getRxCallback()( &data );
        return true;
    }

    bool isRegularMessage( int msgType ) const override {
        return true;
    }
};

// TODO: use FakeServiceTestFixture here
class BluetoothAddonTestSuite : public Test {
public:
    void SetUp() override {
        m_addon.reset( new BluetoothAddon(
            "test",
            std::make_unique< Services::BusWrapper >( Services::BusWrapper::BusTypeTag< TestBus >{} ),
            std::make_unique< HubPlatform::UT::FakeDbClient >(),
            BluetoothAddon::BluetoothManagerTypeTag< MockBluetoothManager >(),
            BluetoothAddon::BluetoothEnvironmentTypeTag< MockBluetoothEnvironment >() ) );

        m_client = std::make_unique< BluetoothClient >( *m_addon );
        m_future = m_helper.runInThread( *m_addon );
    }

    void waitTaskLoop() {
        m_addon->addTask( [this]() { m_addon->stop(); } );

        m_future.get();

        m_helper.clearLoop( *m_addon );
    }

    void TearDown() override {
        m_client.reset();
        m_addon.reset();
    }

    MockBluetoothManager& getManager() {
        return static_cast< MockBluetoothManager& >( m_addon->getManager() );
    }

    BluetoothClient& getClient() {
        return *m_client;
    }

private:
    Services::UT::TaskLoopTestHelper m_helper;
    std::future< void > m_future;

    std::unique_ptr< BluetoothAddon > m_addon;
    std::unique_ptr< BluetoothClient > m_client;
};

TEST_F( BluetoothAddonTestSuite, testStartDiscovery ) {
    EXPECT_CALL( getManager(), startDiscovery() ).Times( 1 );
    getClient().startDiscovery();

    waitTaskLoop();
}

TEST_F( BluetoothAddonTestSuite, testStopDiscovery ) {
    EXPECT_CALL( getManager(), stopDiscovery() ).Times( 1 );
    getClient().stopDiscovery();

    waitTaskLoop();
}

TEST_F( BluetoothAddonTestSuite, testRemoveDevice ) {
    EXPECT_CALL( getManager(), removeDevice( "test_address" ) ).Times( 1 );
    getClient().removeDevice( "test_address" );

    waitTaskLoop();
}

TEST_F( BluetoothAddonTestSuite, testPairDevice ) {
    EXPECT_CALL( getManager(), pair( "test_address" ) ).Times( 1 );
    getClient().pair( "test_address" );

    waitTaskLoop();
}

TEST_F( BluetoothAddonTestSuite, testConnectToDevice ) {
    EXPECT_CALL( getManager(), connectToDevice( "test_address" ) ).Times( 1 );
    getClient().connectToDevice( "test_address" );

    waitTaskLoop();
}

TEST_F( BluetoothAddonTestSuite, testDisconnectDevice ) {
    EXPECT_CALL( getManager(), disconnectDevice( "test_address" ) ).Times( 1 );
    getClient().disconnectDevice( "test_address" );

    waitTaskLoop();
}

TEST_F( BluetoothAddonTestSuite, testWriteCharacteristic ) {
    EXPECT_CALL( getManager(),
                 writeCharacteristic( "test_address", "test_uuid", std::vector< uint8_t >( { 1, 2, 3 } ) ) )
        .Times( 1 );
    getClient().writeCharacteristic( "test_address", "test_uuid", { 1, 2, 3 } );

    waitTaskLoop();
}

TEST_F( BluetoothAddonTestSuite, testReadCharacteristic ) {
    EXPECT_CALL( getManager(), readCharacteristic( "test_address", "test_uuid" ) ).Times( 1 );
    getClient().readCharacteristic( "test_address", "test_uuid" );

    waitTaskLoop();
}

TEST_F( BluetoothAddonTestSuite, testStartNotify ) {
    EXPECT_CALL( getManager(), startNotify( "test_address", "test_uuid" ) ).Times( 1 );
    getClient().startNotify( "test_address", "test_uuid" );

    waitTaskLoop();
}

TEST_F( BluetoothAddonTestSuite, testStopNotify ) {
    EXPECT_CALL( getManager(), stopNotify( "test_address", "test_uuid" ) ).Times( 1 );
    getClient().stopNotify( "test_address", "test_uuid" );

    waitTaskLoop();
}

TEST_F( BluetoothAddonTestSuite, testMaintainConnection ) {
    EXPECT_CALL( getManager(), maintainConnection( "test_address" ) ).Times( 1 );
    getClient().maintainConnection( "test_address" );

    waitTaskLoop();
}

}  // namespace UT
}  // namespace HubPlatform
