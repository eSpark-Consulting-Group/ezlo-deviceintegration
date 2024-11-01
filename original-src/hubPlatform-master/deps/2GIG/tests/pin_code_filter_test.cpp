#include "core/pipeline/pin_code_filter.h"
#include "core/packets/packets.h"

#include "fixtures/pipe_test.h"

namespace TwoGig {
namespace Test {

/*

Test plan

Done    1. Not keyfob
Done    2. Supervisory packet
Done    3. Not pad input
Done    4. Pad

*/

struct PinCodeFilterTest : public PipeTest< PinCodeFilter > {};

TEST_F( PinCodeFilterTest, differentDeviceTypeTest ) {
    Transmission packet = { 0 };
    packet.deviceType = DeviceType::PanicButton;

    pushPayload( packet );
    ASSERT_NE( pullPayload( PinCodeFilter::PipeChannel::StatusUpdate ), nullptr );
    ASSERT_EQ( pullPayload( PinCodeFilter::PipeChannel::PinCodeInput ), nullptr );
}

TEST_F( PinCodeFilterTest, supervisoryTest ) {
    Transmission packet = { 0 };
    new( &packet.status )
        PadStatus{ .keypad = true, .supervisory = true, .lowBattery = 0, .buttonPressed = PadButton::Asterisk };

    pushPayload( packet );
    ASSERT_NE( pullPayload( PinCodeFilter::PipeChannel::StatusUpdate ), nullptr );
    ASSERT_EQ( pullPayload( PinCodeFilter::PipeChannel::PinCodeInput ), nullptr );
}

TEST_F( PinCodeFilterTest, nonPadTest ) {
    Transmission packet = { 0 };
    new( &packet.status )
        PadStatus{ .keypad = true, .supervisory = false, .lowBattery = 0, .buttonPressed = PadButton::Police };

    pushPayload( packet );
    ASSERT_NE( pullPayload( PinCodeFilter::PipeChannel::StatusUpdate ), nullptr );
    ASSERT_EQ( pullPayload( PinCodeFilter::PipeChannel::PinCodeInput ), nullptr );
}

TEST_F( PinCodeFilterTest, padTest ) {
    Transmission packet = { 0 };
    new( &packet.status )
        PadStatus{ .keypad = true, .supervisory = false, .lowBattery = 0, .buttonPressed = PadButton::Asterisk };

    pushPayload( packet );
    ASSERT_EQ( pullPayload( PinCodeFilter::PipeChannel::StatusUpdate ), nullptr );
    ASSERT_NE( pullPayload( PinCodeFilter::PipeChannel::PinCodeInput ), nullptr );
}

}  // namespace Test
}  // namespace TwoGig
