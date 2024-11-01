#include "2gig/device_status.h"
#include "2gig/device_type.h"

#include "core/packets/packets.h"
#include "core/pipeline/packet_duplication_filter.h"

#include "fixtures/pipe_test.h"
#include "fakes/fake_timer.h"

namespace TwoGig {
namespace Test {

/*

Test plan

Done    1. Generic device
Done        1.1. Same device
Done            1.1.1. Same status within interval
Done            1.1.2. Same status outside interval
Done            1.1.3. Different status, within interval
Done            1.1.4. Different status, outside interval
Done        1.2. Different device
Done            1.2.1. Same status within interval
Done            1.2.2. Same status outside interval
Done            1.2.3. Different status, within interval
Done            1.2.4. Different status, outside interval

Done    2. Remote keyfob
Done        2.1. Same button (4) within interval
Done        2.2. Same button (4) outside interval
Done        2.3. Same button (3) within interval
Done        2.4. Same button (3) outside interval

Done    3. Panic button
Done        3.1. Within interval
Done        3.2. Outside interval

Done    4. Door/window sensor
Done        4.1. Within interval
Done        4.2. Outside interval

Done    5. Keypad
Done        5.1. Same sequence within interval
Done        5.2. Same sequence outside interval
Done        5.3. Different sequence within interval
Done        5.4. Different sequence outside interval

*/

static constexpr Timer::Duration WithinIntervalDefault = std::chrono::milliseconds( 1999 );
static constexpr Timer::Duration OutsideIntervalDefault = std::chrono::milliseconds( 2000 );

struct PacketDuplicationFilterTest : public PipeTest< PacketDuplicationFilter > {
    struct PacketBytes {
        uint8_t status;
        uint8_t serial;
    };

public:
    PacketDuplicationFilterTest()
    : PipeTest< PacketDuplicationFilter >( m_timer ) {}

    void testAllowedPacketSequence( PacketBytes firstPacketBytes,
                                    PacketBytes secondPacketBytes,
                                    Timer::Duration interval ) {
        testPackets( DeviceType::PanicButton, firstPacketBytes, secondPacketBytes, interval, true );
    }

    void testNotAllowedPacketSequence( PacketBytes firstPacketBytes,
                                       PacketBytes secondPacketBytes,
                                       Timer::Duration interval ) {
        testPackets( DeviceType::PanicButton, firstPacketBytes, secondPacketBytes, interval, false );
    }

    void testAllowedDeviceSpecificSequence( DeviceType::Enum deviceType, uint8_t status, Timer::Duration interval ) {
        testPackets( deviceType, { status, 0x01 }, { status, 0x01 }, interval, true );
    }

    void testNotAllowedDeviceSpecificSequence( DeviceType::Enum deviceType, uint8_t status, Timer::Duration interval ) {
        testPackets( deviceType, { status, 0x01 }, { status, 0x01 }, interval, false );
    }

protected:
    void testPackets( uint8_t deviceType,
                      PacketBytes firstPacketBytes,
                      PacketBytes secondPacketBytes,
                      Timer::Duration interval,
                      bool shouldBeDispatched ) {
        testPackets( deviceType, deviceType, firstPacketBytes, secondPacketBytes, interval, shouldBeDispatched );
    }

    void testPackets( uint8_t firstDeviceType,
                      uint8_t secondDeviceType,
                      PacketBytes firstPacketBytes,
                      PacketBytes secondPacketBytes,
                      Timer::Duration interval,
                      bool shouldBeDispatched ) {
        auto firstPacket = makePacket( firstPacketBytes.status, firstPacketBytes.serial, firstDeviceType );
        auto secondPacket = makePacket( secondPacketBytes.status, secondPacketBytes.serial, secondDeviceType );

        // Initial
        ASSERT_EQ( pullPayload(), nullptr );

        pushPayload( firstPacket );
        ASSERT_NE( pullPayload(), nullptr );

        resetConsumers();
        m_timer.advanceTime( interval );

        pushPayload( secondPacket );

        if( shouldBeDispatched ) {
            ASSERT_NE( pullPayload(), nullptr );
        }
        else {
            ASSERT_EQ( pullPayload(), nullptr );
        }
    }

private:
    static Transmission makePacket( uint8_t status, uint32_t serial, uint8_t deviceType ) {
        Transmission packet = {};

        packet.status = status;
        packet.serial = serial;
        packet.deviceType = deviceType;

        return packet;
    }

private:
    FakeTimer m_timer;
};

TEST_F( PacketDuplicationFilterTest, sameDeviceSameStatusWithinInterval ) {
    testNotAllowedPacketSequence( { 0x80, 0x01 }, { 0x80, 0x01 }, WithinIntervalDefault );
}

TEST_F( PacketDuplicationFilterTest, sameDeviceSameStatusOutsideInterval ) {
    testAllowedPacketSequence( { 0x80, 0x01 }, { 0x80, 0x01 }, OutsideIntervalDefault );
}

TEST_F( PacketDuplicationFilterTest, sameDeviceDifferentStatusWithinInterval ) {
    testAllowedPacketSequence( { 0x80, 0x01 }, { 0xA0, 0x01 }, WithinIntervalDefault );
}

TEST_F( PacketDuplicationFilterTest, sameDeviceDifferentStatusOutsideInterval ) {
    testAllowedPacketSequence( { 0x80, 0x01 }, { 0xA0, 0x01 }, OutsideIntervalDefault );
}

TEST_F( PacketDuplicationFilterTest, differentDeviceSameStatusWithinInterval ) {
    testAllowedPacketSequence( { 0x80, 0x01 }, { 0x80, 0x02 }, WithinIntervalDefault );
}

TEST_F( PacketDuplicationFilterTest, differentDeviceSameStatusOutsideInterval ) {
    testAllowedPacketSequence( { 0x80, 0x01 }, { 0x80, 0x02 }, OutsideIntervalDefault );
}

TEST_F( PacketDuplicationFilterTest, differentDeviceDifferentStatusWithinInterval ) {
    testAllowedPacketSequence( { 0x80, 0x01 }, { 0xA0, 0x02 }, WithinIntervalDefault );
}

TEST_F( PacketDuplicationFilterTest, differentDeviceDifferentStatusOutsideInterval ) {
    testAllowedPacketSequence( { 0x80, 0x01 }, { 0xA0, 0x02 }, OutsideIntervalDefault );
}

TEST_F( PacketDuplicationFilterTest, keyfobSameButton4WithinInterval ) {
    testNotAllowedDeviceSpecificSequence( DeviceType::RemoteKeyFob, 0x80, std::chrono::milliseconds( 99 ) );
}

TEST_F( PacketDuplicationFilterTest, keyfobSameButton4OutsideInterval ) {
    testAllowedDeviceSpecificSequence( DeviceType::RemoteKeyFob, 0x80, std::chrono::milliseconds( 100 ) );
}

TEST_F( PacketDuplicationFilterTest, keyfobSameButton3WithinInterval ) {
    testNotAllowedDeviceSpecificSequence( DeviceType::RemoteKeyFob, 0x40, std::chrono::milliseconds( 799 ) );
}

TEST_F( PacketDuplicationFilterTest, keyfobSameButton3OutsideInterval ) {
    testAllowedDeviceSpecificSequence( DeviceType::RemoteKeyFob, 0x40, std::chrono::milliseconds( 800 ) );
}

TEST_F( PacketDuplicationFilterTest, panicButtonWithinInterval ) {
    testNotAllowedDeviceSpecificSequence( DeviceType::PanicButton, 0x80, std::chrono::milliseconds( 1999 ) );
}

TEST_F( PacketDuplicationFilterTest, panicButtonOutsideInterval ) {
    testAllowedDeviceSpecificSequence( DeviceType::PanicButton, 0x80, std::chrono::milliseconds( 2000 ) );
}

TEST_F( PacketDuplicationFilterTest, doorWindowSensorWithinInterval ) {
    testNotAllowedDeviceSpecificSequence( DeviceType::DoorWindowSensor, 0x20, std::chrono::milliseconds( 1999 ) );
}

TEST_F( PacketDuplicationFilterTest, doorWindowSensorOutsideInterval ) {
    testAllowedDeviceSpecificSequence( DeviceType::DoorWindowSensor, 0x20, std::chrono::milliseconds( 2000 ) );
}

TEST_F( PacketDuplicationFilterTest, keypadSameSequenceWithinInterval ) {
    testPackets( 1, 1, { 0x01, 0x01 }, { 0x01, 0x01 }, std::chrono::milliseconds( 199 ), false );
}

TEST_F( PacketDuplicationFilterTest, keypadSameSequenceOutsideInterval ) {
    testPackets( 1, 1, { 0x01, 0x01 }, { 0x01, 0x01 }, std::chrono::milliseconds( 201 ), false );
}

TEST_F( PacketDuplicationFilterTest, keypadDifferentSequenceWithinInterval ) {
    testPackets( 1, 2, { 0x01, 0x01 }, { 0x01, 0x01 }, std::chrono::milliseconds( 199 ), true );
}

TEST_F( PacketDuplicationFilterTest, keypadDifferentSequenceOutsideInterval ) {
    testPackets( 1, 2, { 0x01, 0x01 }, { 0x01, 0x01 }, std::chrono::milliseconds( 201 ), true );
}

}  // namespace Test
}  // namespace TwoGig
