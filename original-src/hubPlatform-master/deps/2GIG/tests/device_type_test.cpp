#include "core/packets/packets.h"

#include <gtest/gtest.h>

namespace TwoGig {
namespace Test {

/*

Test plan

Done    1. Regular type
Done    2. Honeywell
Done    3. Keypad

*/

struct DeviceTypeTest : public testing::Test {};

TEST_F( DeviceTypeTest, regularType ) {
    Transmission packet = {};
    packet.deviceType = DeviceType::PanicButton;

    ASSERT_EQ( packet.getDeviceType(), DeviceType::PanicButton );
}

TEST_F( DeviceTypeTest, honeywellType ) {
    Transmission packet = {};
    packet.deviceType = DeviceType::HoneywellSensor;

    ASSERT_EQ( packet.getDeviceType(), DeviceType::HoneywellSensor );
}

TEST_F( DeviceTypeTest, keypadType ) {
    Transmission packet = {};
    packet.status = 0x01;

    ASSERT_EQ( packet.getDeviceType(), DeviceType::Keypad );
}

}  // namespace Test
}  // namespace TwoGig
