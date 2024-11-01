#include "core/packets/packets.h"
#include "core/pipeline/device_collector.h"

#include "utils/logger/logger.h"

#include "fixtures/pipe_test.h"
#include "fakes/fake_client.h"

namespace TwoGig {
namespace Test {

/*

Test plan

Done    1. Same device
Done    2. Different device

*/

struct DeviceCollectorTest : public testing::Test, public FakeClient {
    DeviceCollectorTest()
    : m_deviceCollector( m_logger, *this ) {}

    void onNewDeviceDiscovered( uint32_t serial, DeviceType::Enum deviceType ) override {
        m_discoveredDevices.emplace_back( serial, deviceType );
    }

    void checkDiscoveredDevices( const std::vector< std::pair< uint32_t, DeviceType::Enum > >& expected ) const {
        ASSERT_EQ( m_discoveredDevices.size(), expected.size() );

        for( int i = 0; i != m_discoveredDevices.size(); ++i ) {
            ASSERT_EQ( m_discoveredDevices[ i ], expected[ i ] );
        }
    }

    void processPacket( uint32_t serial, DeviceType::Enum deviceType ) {
        Transmission packet = {};

        packet.serial = serial;
        packet.deviceType = deviceType;

        m_deviceCollector.processPayload( packet );
    }

private:
    Logger m_logger;

    std::vector< std::pair< uint32_t, DeviceType::Enum > > m_discoveredDevices;

protected:
    DeviceCollector m_deviceCollector;
};

TEST_F( DeviceCollectorTest, sameDeviceTest ) {
    // Initial
    checkDiscoveredDevices( {} );

    processPacket( 2, DeviceType::PanicButton );
    checkDiscoveredDevices( { { 2, DeviceType::PanicButton } } );

    processPacket( 2, DeviceType::PanicButton );
    checkDiscoveredDevices( { { 2, DeviceType::PanicButton } } );
}

TEST_F( DeviceCollectorTest, differentDeviceTest ) {
    // Initial
    checkDiscoveredDevices( {} );

    processPacket( 2, DeviceType::PanicButton );
    checkDiscoveredDevices( { { 2, DeviceType::PanicButton } } );

    processPacket( 3, DeviceType::PanicButton );
    checkDiscoveredDevices( { { 2, DeviceType::PanicButton }, { 3, DeviceType::PanicButton } } );
}

}  // namespace Test
}  // namespace TwoGig
