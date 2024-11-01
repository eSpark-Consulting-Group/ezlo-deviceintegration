#include "core/pipeline/packet_converter.h"

#include "fixtures/pipe_test.h"

namespace TwoGig {
namespace Test {

/*

Test plan

Done    1. Transmission
Done    2. Jamming
Done    3. Power On
Done    4. Supervisory

*/

template < typename PacketT >
struct PacketConverterTest : public PipeTest< PacketConverter< PacketT > > {};

using TransmissionConverterTest = PacketConverterTest< Transmission >;
using JammingConverterTest = PacketConverterTest< Jamming >;
using PowerOnConverterTest = PacketConverterTest< PowerOn >;
using SupervisoryConverterTest = PacketConverterTest< Supervisory >;

TEST_F( TransmissionConverterTest, transmissionTest ) {
    // Initial
    ASSERT_EQ( pullPayload(), nullptr );

    pushPayload( { 0x55, 0xAE, 0x82, 0xF8, 0x3E, 0x53, 0xA2, 0xA0 } );

    const Transmission* packet = pullPayload();
    ASSERT_NE( packet, nullptr );

    ASSERT_EQ( packet->header, 0x55 );
    ASSERT_EQ( packet->packetId, 0xAE );
    ASSERT_EQ( packet->status, 0x82 );
    ASSERT_EQ( packet->deviceType, DeviceType::RemoteKeyFob );
    ASSERT_EQ( packet->serial, 0x00083E53 );
    ASSERT_EQ( packet->signalStrength, -94 );
    ASSERT_EQ( packet->crc, 0xA0 );
}

TEST_F( JammingConverterTest, jammingTest ) {
    // Initial
    ASSERT_EQ( pullPayload(), nullptr );

    pushPayload( { 0x55, 0x57, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA7 } );

    const Jamming* packet = pullPayload();
    ASSERT_NE( packet, nullptr );

    ASSERT_EQ( packet->header, 0x55 );
    ASSERT_EQ( packet->packetId, 0x57 );
    ASSERT_EQ( packet->crc, 0xA7 );
}

TEST_F( PowerOnConverterTest, powerOnTest ) {
    // Initial
    ASSERT_EQ( pullPayload(), nullptr );

    pushPayload( { 0x55, 0x1F, 0x06, 0x00, 0x15, 0xEA, 0x00, 0x9D } );

    const PowerOn* packet = pullPayload();
    ASSERT_NE( packet, nullptr );

    ASSERT_EQ( packet->header, 0x55 );
    ASSERT_EQ( packet->packetId, 0x1F );
    ASSERT_EQ( packet->firmwareVersionMajor, 0x06 );
    ASSERT_EQ( packet->firmwareVersionMinor, 0x00 );
    ASSERT_EQ( packet->serial, 0x15EA00 );
    ASSERT_EQ( packet->crc, 0x9D );
}

TEST_F( SupervisoryConverterTest, supervisoryTest ) {
    // Initial
    ASSERT_EQ( pullPayload(), nullptr );

    pushPayload( { 0x37 } );

    const Supervisory* packet = pullPayload();
    ASSERT_NE( packet, nullptr );

    ASSERT_EQ( packet->header, 0x37 );
}

}  // namespace Test
}  // namespace TwoGig
