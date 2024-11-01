#include "core/pipeline/packet_validator.h"

#include "fixtures/pipe_test.h"

namespace TwoGig {
namespace Test {

/*

Test plan

Done    1. Invalid
Done    2. Valid

*/

struct PacketValidatorTest : public PipeTest< PacketValidator > {};

TEST_F( PacketValidatorTest, invalidPacketTest ) {
    // Initial
    ASSERT_EQ( pullPayload(), nullptr );

    pushPayload( { 0x55, 0xAE, 0x82, 0xF8, 0x3E, 0x53, 0xA2, 0xA1 } );
    ASSERT_EQ( pullPayload(), nullptr );
}

TEST_F( PacketValidatorTest, validPacketTest ) {
    // Initial
    ASSERT_EQ( pullPayload(), nullptr );

    pushPayload( { 0x55, 0xAE, 0x82, 0xF8, 0x3E, 0x53, 0xA2, 0xA0 } );
    ASSERT_NE( pullPayload(), nullptr );
}

}  // namespace Test
}  // namespace TwoGig
