#include "core/packets/packets.h"
#include "core/pipeline/device_filter.h"

#include "fixtures/pipe_test.h"
#include "fakes/fake_devices_repository.h"

namespace TwoGig {
namespace Test {

/*

Test plan

Done    1. Included device
Done    2. Excluded device

*/

struct DeviceFilterTest : public PipeTest< DeviceFilter > {
    DeviceFilterTest()
    : PipeTest< DeviceFilter >( m_devices ) {
        m_devices.addDevice( { 1 } );
    }

private:
    FakeDevicesRepository m_devices;
};

TEST_F( DeviceFilterTest, includedDeviceTest ) {
    Transmission packet = {};
    packet.serial = 1;

    // Initial
    ASSERT_EQ( pullPayload(), nullptr );

    pushPayload( packet );
    ASSERT_NE( pullPayload(), nullptr );
}

TEST_F( DeviceFilterTest, excludedDeviceTeset ) {
    Transmission packet = {};
    packet.serial = 2;

    // Initial
    ASSERT_EQ( pullPayload(), nullptr );

    pushPayload( packet );
    ASSERT_EQ( pullPayload(), nullptr );
}

}  // namespace Test
}  // namespace TwoGig
