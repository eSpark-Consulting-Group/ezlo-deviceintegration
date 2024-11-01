#include "utils/byte_cast.h"

#include "2gig/device_status.h"

#include <gtest/gtest.h>

namespace TwoGig {
namespace Test {

/*

Test plan

Done    1. Button pressed
Done    2. Supervisory

*/

struct ByteCastTest : public testing::Test {};

TEST_F( ByteCastTest, buttonPressedEvent ) {
    // hex : 0x80 ; Supervisory event
    std::uint8_t panicButtonBytes = 0b10000000;
    const auto panicButtonInfo = fromBytes< PanicButtonStatus >( &panicButtonBytes );

    EXPECT_TRUE( panicButtonInfo.buttonPressed ) << "Button should be pressed!";
    EXPECT_FALSE( panicButtonInfo.lowBattery ) << "Battery should be ok.";
}

TEST_F( ByteCastTest, supervisoryEvent ) {
    // hex : 0x04 ; Supervisory event
    std::uint8_t panicButtonBytes = 0b00000100;
    const auto panicButtonInfo = fromBytes< PanicButtonStatus >( &panicButtonBytes );

    EXPECT_FALSE( panicButtonInfo.buttonPressed ) << "Button wasn't pressed. It's just Supervisory event!";
    EXPECT_FALSE( panicButtonInfo.lowBattery ) << "Battery should be ok. It's just Supervisory event!";
}

}  // namespace Test
}  // namespace TwoGig
