#include "core/pipeline/packet_collector.h"

#include "fixtures/pipe_test.h"

namespace TwoGig {
namespace Test {

/*

Test plan

Done    1. Unknown
Done    2. Supervisory
Done    3. Transmission 8 bytes
Done    4. Transmission 11 bytes

*/

struct PacketCollectorTest : public PipeTest< PacketCollector > {
    void testSerialPacket( const PacketBytes& packetBytes ) {
        for( const uint8_t byte : packetBytes ) {
            ASSERT_EQ( pullPayload(), nullptr );
            pushPayload( byte );
        }

        auto packet = pullPayload();

        ASSERT_NE( packet, nullptr );
        ASSERT_EQ( *packet, packetBytes );
    }
};

TEST_F( PacketCollectorTest, unknownPacketTest ) {
    // Initial
    ASSERT_EQ( pullPayload(), nullptr );

    pushPayload( 0x00 );
    ASSERT_EQ( pullPayload(), nullptr );
}

TEST_F( PacketCollectorTest, supervisoryPacketTest ) {
    testSerialPacket( { 0x37 } );
}

TEST_F( PacketCollectorTest, transmission8BytePacketTest ) {
    testSerialPacket( { 0x55, 0xAE, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 } );
}

TEST_F( PacketCollectorTest, transmission11BytePacketTest ) {
    testSerialPacket( { 0x55, 0xB0, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B } );
}

}  // namespace Test
}  // namespace TwoGig
