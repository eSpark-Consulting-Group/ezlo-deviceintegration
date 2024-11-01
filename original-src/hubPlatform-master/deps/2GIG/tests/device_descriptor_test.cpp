#include "2gig/device_descriptor.h"

#include <gtest/gtest.h>

namespace TwoGig {
namespace Test {

/*

Test plan

Done    1. With model
Done    2. With device type

*/

struct DeviceDescriptorTest : public testing::Test {};

TEST_F( DeviceDescriptorTest, withModelTest ) {
    DeviceDescriptor descriptor{ { 123 }, "foo" };
    ASSERT_EQ( descriptor.getType(), DeviceDescriptor::DescriptorType::Model );
    ASSERT_EQ( descriptor.getModel(), "foo" );

    auto copy = descriptor;
    ASSERT_EQ( copy.getType(), DeviceDescriptor::DescriptorType::Model );
    ASSERT_EQ( copy.getModel(), "foo" );
}

TEST_F( DeviceDescriptorTest, withDeviceTypeTest ) {
    DeviceDescriptor descriptor{ { 123 }, DeviceType::DoorWindowSensor };
    ASSERT_EQ( descriptor.getType(), DeviceDescriptor::DescriptorType::DeviceType );
    ASSERT_EQ( descriptor.getDeviceType(), DeviceType::DoorWindowSensor );

    auto copy = descriptor;
    ASSERT_EQ( copy.getType(), DeviceDescriptor::DescriptorType::DeviceType );
    ASSERT_EQ( copy.getDeviceType(), DeviceType::DoorWindowSensor );
}

}  // namespace Test
}  // namespace TwoGig
