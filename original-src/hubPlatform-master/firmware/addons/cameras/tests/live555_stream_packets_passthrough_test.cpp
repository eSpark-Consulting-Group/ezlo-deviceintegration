#include "cameras/rtsp_server/live555/stream_packet_passthrough.h"
#include "cameras/rtsp_server/live555/stream_framed_source.h"
#include "cameras/packet/packet_buffer.h"
#include "cameras/packet/packet_publisher.h"

#include "system_utils/find_free_local_port.h"
#include "log_utils/ezlo_log.h"
#include "common_utils/cpp14.h"

#include "BasicUsageEnvironment.hh"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace ::testing;

namespace HubPlatform {
namespace Cameras {
namespace Live555 {
namespace UT {

/*

    TEST PLAN
DONE    1.1. All pre-cached packets are provided to framed source
DONE    1.2. All packets from input are provided to framed source
DONE    1.3. Stream packets before the key frame's packet are discarded
DONE    1.4. Pre-cached packets are provided to multiple sources

 */

constexpr int StreamIndex = 0;

class TestStreamFramedSourceDeleter {
public:
    void operator()( StreamFramedSource* ptr ) {
        delete ptr;
    }
};
using TestStreamFramedSourceUPtr = std::unique_ptr< StreamFramedSource, TestStreamFramedSourceDeleter >;

static bool operator==( const Packet& lhs, const Packet& rhs ) {
    return lhs.data == rhs.data && lhs.timestamp == rhs.timestamp && lhs.status == rhs.status &&
           lhs.streamIndex == rhs.streamIndex && lhs.isKey == rhs.isKey;
}

class StreamFramedSourceTests : public Test {
public:
    void SetUp() override {
        Log::setLocalLogLevel( Log::PriorityLevel::TRACE );

        m_taskScheduler = std::unique_ptr< TaskScheduler >( BasicTaskScheduler::createNew() );
        m_env = UsageEnvironmentUniquePtr( BasicUsageEnvironment::createNew( *m_taskScheduler ),
                                           []( UsageEnvironment* ptr ) { ptr->reclaim(); } );

        m_packetPublisher = std::make_unique< PacketPublisher >();
        m_packetsPassthrough = std::make_unique< StreamPacketPassthrough >( *m_packetPublisher );
    }

    IPacketPublisher& getPacketPublisher() const {
        return *m_packetPublisher;
    }

    Packet createPacket( uint8_t dataValues, int streamIndex, bool isKeyPacket ) const {
        constexpr int TestDataSize = 10;
        Packet packet;
        packet.data = std::vector< uint8_t >( TestDataSize, dataValues );
        packet.timestamp = std::chrono::nanoseconds( 0 );
        packet.status = Status::RUN;
        packet.streamIndex = streamIndex;
        packet.isKey = isKeyPacket;
        return packet;
    }

    TestStreamFramedSourceUPtr createStreamFramedSource( std::unique_ptr< IPacketBuffer > packetBuffer,
                                                         int streamIndex ) const {
        return TestStreamFramedSourceUPtr( StreamFramedSource::createNew(
            *m_env, *m_packetsPassthrough, std::move( packetBuffer ), "streamId", streamIndex, true ) );
    }

private:
    using UsageEnvironmentUniquePtr = std::unique_ptr< UsageEnvironment, std::function< void( UsageEnvironment* ) > >;
    std::unique_ptr< TaskScheduler > m_taskScheduler;
    UsageEnvironmentUniquePtr m_env;

    std::unique_ptr< PacketPublisher > m_packetPublisher;
    std::unique_ptr< StreamPacketPassthrough > m_packetsPassthrough;
};

TEST_F( StreamFramedSourceTests, precachedStreamPacketsAreProvidedToSource ) {  // 1.1
    IPacketPublisher& packetPublisher = getPacketPublisher();
    const Packet inputPacket1 = createPacket( 101, StreamIndex, true );
    const Packet inputPacket2 = createPacket( 202, StreamIndex, false );
    packetPublisher.sendPacket( inputPacket1 );
    packetPublisher.sendPacket( inputPacket2 );

    auto packetBuffer = std::make_unique< PacketBuffer >( 10 );
    IPacketBuffer* packetBufferPtr = packetBuffer.get();
    auto framedSource = createStreamFramedSource( std::move( packetBuffer ), StreamIndex );

    Packet outputPacket1;
    Packet outputPacket2;
    EXPECT_TRUE( packetBufferPtr->pop( outputPacket1 ) );
    EXPECT_TRUE( packetBufferPtr->pop( outputPacket2 ) );
    EXPECT_TRUE( inputPacket1 == outputPacket1 );
    EXPECT_TRUE( inputPacket2 == outputPacket2 );
}

TEST_F( StreamFramedSourceTests, livePacketsAreProvidedToSource ) {  // 1.2
    IPacketPublisher& packetPublisher = getPacketPublisher();
    auto packetBuffer = std::make_unique< PacketBuffer >( 10 );
    IPacketBuffer* packetBufferPtr = packetBuffer.get();
    auto framedSource = createStreamFramedSource( std::move( packetBuffer ), StreamIndex );

    const Packet inputPacket1 = createPacket( 101, StreamIndex, true );
    const Packet inputPacket2 = createPacket( 202, StreamIndex, false );
    packetPublisher.sendPacket( inputPacket1 );
    packetPublisher.sendPacket( inputPacket2 );

    Packet outputPacket1;
    Packet outputPacket2;
    EXPECT_TRUE( packetBufferPtr->pop( outputPacket1 ) );
    EXPECT_TRUE( packetBufferPtr->pop( outputPacket2 ) );
    EXPECT_TRUE( inputPacket1 == outputPacket1 );
    EXPECT_TRUE( inputPacket2 == outputPacket2 );
}

TEST_F( StreamFramedSourceTests, packetsBeforeKeyFrameAreDiscarded ) {  // 1.3
    IPacketPublisher& packetPublisher = getPacketPublisher();
    const Packet inputPacket1 = createPacket( 101, StreamIndex, false );
    const Packet inputPacket2 = createPacket( 202, StreamIndex, true );
    packetPublisher.sendPacket( inputPacket1 );
    packetPublisher.sendPacket( inputPacket2 );

    auto packetBuffer = std::make_unique< PacketBuffer >( 10 );
    IPacketBuffer* packetBufferPtr = packetBuffer.get();
    auto framedSource = createStreamFramedSource( std::move( packetBuffer ), StreamIndex );

    Packet outputPacket1;
    Packet outputPacket2;
    EXPECT_TRUE( packetBufferPtr->pop( outputPacket1 ) );
    EXPECT_TRUE( outputPacket1 == inputPacket2 );
    EXPECT_FALSE( packetBufferPtr->pop( outputPacket2 ) );
}

TEST_F( StreamFramedSourceTests, precachedPacketsProvidedToMulitpleSources ) {  // 1.4
    IPacketPublisher& packetPublisher = getPacketPublisher();
    const Packet inputPacket1 = createPacket( 101, StreamIndex, true );
    const Packet inputPacket2 = createPacket( 202, StreamIndex, false );
    packetPublisher.sendPacket( inputPacket1 );
    packetPublisher.sendPacket( inputPacket2 );

    auto packetBuffer1 = std::make_unique< PacketBuffer >( 10 );
    IPacketBuffer* packetBuffer1Ptr = packetBuffer1.get();
    auto framedSource1 = createStreamFramedSource( std::move( packetBuffer1 ), StreamIndex );
    auto packetBuffer2 = std::make_unique< PacketBuffer >( 10 );
    IPacketBuffer* packetBuffer2Ptr = packetBuffer2.get();
    auto framedSource2 = createStreamFramedSource( std::move( packetBuffer2 ), StreamIndex );

    Packet source1OutputPacket1;
    Packet source1OutputPacket2;
    EXPECT_TRUE( packetBuffer1Ptr->pop( source1OutputPacket1 ) );
    EXPECT_TRUE( packetBuffer1Ptr->pop( source1OutputPacket2 ) );
    EXPECT_TRUE( inputPacket1 == source1OutputPacket1 );
    EXPECT_TRUE( inputPacket2 == source1OutputPacket2 );

    Packet source2OutputPacket1;
    Packet source2OutputPacket2;
    EXPECT_TRUE( packetBuffer2Ptr->pop( source2OutputPacket1 ) );
    EXPECT_TRUE( packetBuffer2Ptr->pop( source2OutputPacket2 ) );
    EXPECT_TRUE( inputPacket1 == source2OutputPacket1 );
    EXPECT_TRUE( inputPacket2 == source2OutputPacket2 );

    EXPECT_TRUE( source1OutputPacket1 == source2OutputPacket1 );
    EXPECT_TRUE( source1OutputPacket2 == source2OutputPacket2 );
}

}  // namespace UT
}  // namespace Live555
}  // namespace Cameras
}  // namespace HubPlatform
