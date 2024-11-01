#include "core/pipeline/packet_sorter.h"
#include "core/packets/packet_id.h"
#include "core/packets/packet_type.h"

#include "fixtures/pipe_test.h"

namespace TwoGig {
namespace Test {

/*

Test plan

Done    1. Unknown packet type
Done    2. Unknown packet id
Done    3. Supervisory
Done    4. 8 byte transmission
Done    5. Honeywell transmission
Done    6. 11 byte transmission
Done    7. Jam detected
Done    8. Jam restore
Done    9. Power on

*/

struct PacketSorterTest : public PipeTest< PacketSorter > {
    void testPacket( uint8_t packetType, uint8_t packetId, size_t channel ) {
        for( size_t i = 0; i != PacketSorter::channelsCount; ++i ) {
            ASSERT_EQ( pullPayload( i ), nullptr );
        }

        // Note: validity of the packet doesn't matter for this pipe
        pushPayload( { packetType, packetId } );
        for( size_t i = 0; i != PacketSorter::channelsCount; ++i ) {
            if( i != channel )
                ASSERT_EQ( pullPayload( i ), nullptr );
        }

        ASSERT_NE( pullPayload( channel ), nullptr );
    }
};

TEST_F( PacketSorterTest, unknownPacketTypeTest ) {
    testPacket( 0x00, PacketId::PowerOn, PacketSorter::PipeChannel::Unknown );
}

TEST_F( PacketSorterTest, unknownPacketIdTest ) {
    testPacket( PacketType::Regular, 0x00, PacketSorter::PipeChannel::Unknown );
}

TEST_F( PacketSorterTest, supervisoryPacketTest ) {
    testPacket( PacketType::Supervisory, 0x00, PacketSorter::PipeChannel::Supervisory );
}

TEST_F( PacketSorterTest, twoGig8ByteTransmissionPacketTest ) {
    testPacket( PacketType::Regular, PacketId::TwoGig8ByteTransmission, PacketSorter::PipeChannel::Transmission );
}

TEST_F( PacketSorterTest, honeywellTransmissionPacketTest ) {
    testPacket( PacketType::Regular, PacketId::HoneywellTransmission, PacketSorter::PipeChannel::Transmission );
}

TEST_F( PacketSorterTest, twoGig11ByteTransmissionPacketTest ) {
    testPacket(
        PacketType::Regular, PacketId::TwoGig11ByteTransmission, PacketSorter::PipeChannel::EncryptedTransmission );
}

TEST_F( PacketSorterTest, jamDetectPacketTest ) {
    testPacket( PacketType::Regular, PacketId::JamDetect, PacketSorter::PipeChannel::JamDetect );
}

TEST_F( PacketSorterTest, jamRestorePacketTest ) {
    testPacket( PacketType::Regular, PacketId::JamRestore, PacketSorter::PipeChannel::JamRestore );
}

TEST_F( PacketSorterTest, powerOnPacketTest ) {
    testPacket( PacketType::Regular, PacketId::PowerOn, PacketSorter::PipeChannel::PowerOn );
}

}  // namespace Test
}  // namespace TwoGig
