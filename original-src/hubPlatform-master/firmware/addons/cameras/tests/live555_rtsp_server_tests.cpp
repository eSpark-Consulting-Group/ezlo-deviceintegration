#include "cameras/rtsp_server/live555/rtsp_server_controller.h"
#include "cameras/codec/video_codec.h"
#include "cameras/events/event_publisher_impl.h"

#include "system_utils/find_free_local_port.h"

#include "log_utils/ezlo_log.h"

#include "common_utils/cpp14.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <thread>
#include <future>

using namespace ::testing;

namespace HubPlatform {
namespace Cameras {
namespace Live555 {
namespace UT {

/*

    TEST PLAN
DONE    1.1. Create streams and stop server immediately
DONE    1.2. Create streams and delete streams in destructor
DONE    1.3. Create and delete immediately one stream
DONE    1.4. Create and delete immediately several streams
DONE    1.5. Create and delete sequentially one streams
DONE    1.6. Create and delete sequentially several streams
DONE    1.7. Wait for server start and stop server
DONE    1.8. Start stop server with stream creating
DONE    1.9. Create and stop server after delay


 */

const static std::chrono::seconds WaitTimeoutSec = std::chrono::seconds( 5 );
const static size_t PacketCountLimit = 500;

class MockIPacketPublisher : public IPacketPublisher {
public:
    MOCK_METHOD1( sendPacket, void( const Packet& ) );
    MOCK_METHOD1( addListener, ListenerId( PacketHandlerFunc ) );
    MOCK_METHOD1( removeListener, void( const ListenerId& ) );
};

class MockIEventPublisher : public IEventPublisher< EventBase > {
public:
    MOCK_METHOD1( sendEvent, void( EventBase ) );
    MOCK_METHOD1( addListener, ListenerId( EventHandler ) );
    MOCK_METHOD1( removeListener, void( const ListenerId& ) );
};

class RtspServerControllerTests : public Test {
public:
    void SetUp() override {
        Log::setLocalLogLevel( Log::PriorityLevel::TRACE );
    }

    RtspServerController::Codecs getCodecs() {
        RtspServerController::Codecs codecs;
        codecs.emplace_back( std::make_unique< VideoCodec >( 0, 0, CodecType::VIDEO_H264, "VIDEO_H264" ) );
        return codecs;
    };
};

TEST_F( RtspServerControllerTests, createAndStopServerImmediately ) {  // 1.1
    MockIPacketPublisher packetPublisher;
    MockIEventPublisher eventPublisher;

    RtspServerController rtspServer( system_utils::getUniqueFreeLocalPort(), PacketCountLimit, eventPublisher );
    rtspServer.start();
    ASSERT_TRUE( rtspServer.waitForStart( WaitTimeoutSec ) );

    EXPECT_CALL( eventPublisher, sendEvent( EventBase( "streamId1", EventType::STREAM_CREATED ) ) );
    EXPECT_CALL( eventPublisher, sendEvent( EventBase( "streamId2", EventType::STREAM_CREATED ) ) );
    EXPECT_CALL( eventPublisher, sendEvent( EventBase( "streamId3", EventType::STREAM_CREATED ) ) );
    EXPECT_CALL( eventPublisher, sendEvent( EventBase( "streamId4", EventType::STREAM_CREATED ) ) );
    EXPECT_CALL( eventPublisher, sendEvent( EventBase( "streamId5", EventType::STREAM_CREATED ) ) );

    EXPECT_CALL( eventPublisher, sendEvent( EventBase( "streamId1", EventType::STREAM_DELETED ) ) );
    EXPECT_CALL( eventPublisher, sendEvent( EventBase( "streamId2", EventType::STREAM_DELETED ) ) );
    EXPECT_CALL( eventPublisher, sendEvent( EventBase( "streamId3", EventType::STREAM_DELETED ) ) );
    EXPECT_CALL( eventPublisher, sendEvent( EventBase( "streamId4", EventType::STREAM_DELETED ) ) );
    EXPECT_CALL( eventPublisher, sendEvent( EventBase( "streamId5", EventType::STREAM_DELETED ) ) );

    rtspServer.createCameraStream( packetPublisher, "streamId1", getCodecs(), "" );
    rtspServer.createCameraStream( packetPublisher, "streamId2", getCodecs(), "" );
    rtspServer.createCameraStream( packetPublisher, "streamId3", getCodecs(), "" );
    rtspServer.createCameraStream( packetPublisher, "streamId4", getCodecs(), "" );
    rtspServer.createCameraStream( packetPublisher, "streamId5", getCodecs(), "" );
    EXPECT_EQ( rtspServer.getStreamQuantity(), 5 );
    rtspServer.stop();
    EXPECT_EQ( rtspServer.getStreamQuantity(), 0 );
}

TEST_F( RtspServerControllerTests, createAndDeleteInDestructor ) {  // 1.2
    MockIPacketPublisher packetPublisher;
    MockIEventPublisher eventPublisher;

    EXPECT_CALL( eventPublisher, sendEvent( EventBase( "streamId1", EventType::STREAM_CREATED ) ) );
    EXPECT_CALL( eventPublisher, sendEvent( EventBase( "streamId2", EventType::STREAM_CREATED ) ) );
    EXPECT_CALL( eventPublisher, sendEvent( EventBase( "streamId3", EventType::STREAM_CREATED ) ) );
    EXPECT_CALL( eventPublisher, sendEvent( EventBase( "streamId4", EventType::STREAM_CREATED ) ) );
    EXPECT_CALL( eventPublisher, sendEvent( EventBase( "streamId5", EventType::STREAM_CREATED ) ) );

    EXPECT_CALL( eventPublisher, sendEvent( EventBase( "streamId1", EventType::STREAM_DELETED ) ) );
    EXPECT_CALL( eventPublisher, sendEvent( EventBase( "streamId2", EventType::STREAM_DELETED ) ) );
    EXPECT_CALL( eventPublisher, sendEvent( EventBase( "streamId3", EventType::STREAM_DELETED ) ) );
    EXPECT_CALL( eventPublisher, sendEvent( EventBase( "streamId4", EventType::STREAM_DELETED ) ) );
    EXPECT_CALL( eventPublisher, sendEvent( EventBase( "streamId5", EventType::STREAM_DELETED ) ) );
    {
        RtspServerController rtspServer( system_utils::getUniqueFreeLocalPort(), PacketCountLimit, eventPublisher );
        rtspServer.start();
        ASSERT_TRUE( rtspServer.waitForStart( WaitTimeoutSec ) );

        rtspServer.createCameraStream( packetPublisher, "streamId1", getCodecs(), "" );
        rtspServer.createCameraStream( packetPublisher, "streamId2", getCodecs(), "" );
        rtspServer.createCameraStream( packetPublisher, "streamId3", getCodecs(), "" );
        rtspServer.createCameraStream( packetPublisher, "streamId4", getCodecs(), "" );
        rtspServer.createCameraStream( packetPublisher, "streamId5", getCodecs(), "" );
        EXPECT_EQ( rtspServer.getStreamQuantity(), 5 );
    }
}

TEST_F( RtspServerControllerTests, createAndDeleteImmediatelyOneStream ) {  // 1.3
    MockIPacketPublisher packetPublisher;
    MockIEventPublisher eventPublisher;

    EXPECT_CALL( eventPublisher, sendEvent( EventBase( "streamId1", EventType::STREAM_CREATED ) ) );
    EXPECT_CALL( eventPublisher, sendEvent( EventBase( "streamId1", EventType::STREAM_DELETED ) ) );

    RtspServerController rtspServer( system_utils::getUniqueFreeLocalPort(), PacketCountLimit, eventPublisher );
    rtspServer.start();
    ASSERT_TRUE( rtspServer.waitForStart( WaitTimeoutSec ) );

    rtspServer.createCameraStream( packetPublisher, "streamId1", getCodecs(), "" );
    rtspServer.deleteCameraStream( "streamId1" );

    rtspServer.createCameraStream( packetPublisher, "streamId1", getCodecs(), "" );
    rtspServer.deleteCameraStream( "streamId1" );

    rtspServer.createCameraStream( packetPublisher, "streamId1", getCodecs(), "" );
    rtspServer.deleteCameraStream( "streamId1" );

    rtspServer.createCameraStream( packetPublisher, "streamId1", getCodecs(), "" );
    rtspServer.deleteCameraStream( "streamId1" );
}

TEST_F( RtspServerControllerTests, createAndDeleteImmediatelySeveralStreams ) {  // 1.4
    MockIPacketPublisher packetPublisher;
    MockIEventPublisher eventPublisher;

    EXPECT_CALL( eventPublisher, sendEvent( EventBase( "streamId1", EventType::STREAM_CREATED ) ) );
    EXPECT_CALL( eventPublisher, sendEvent( EventBase( "streamId2", EventType::STREAM_CREATED ) ) );
    EXPECT_CALL( eventPublisher, sendEvent( EventBase( "streamId3", EventType::STREAM_CREATED ) ) );
    EXPECT_CALL( eventPublisher, sendEvent( EventBase( "streamId4", EventType::STREAM_CREATED ) ) );

    EXPECT_CALL( eventPublisher, sendEvent( EventBase( "streamId1", EventType::STREAM_DELETED ) ) );
    EXPECT_CALL( eventPublisher, sendEvent( EventBase( "streamId2", EventType::STREAM_DELETED ) ) );
    EXPECT_CALL( eventPublisher, sendEvent( EventBase( "streamId3", EventType::STREAM_DELETED ) ) );
    EXPECT_CALL( eventPublisher, sendEvent( EventBase( "streamId4", EventType::STREAM_DELETED ) ) );

    RtspServerController rtspServer( system_utils::getUniqueFreeLocalPort(), PacketCountLimit, eventPublisher );
    rtspServer.start();
    ASSERT_TRUE( rtspServer.waitForStart( WaitTimeoutSec ) );

    rtspServer.createCameraStream( packetPublisher, "streamId1", getCodecs(), "" );
    rtspServer.deleteCameraStream( "streamId1" );

    rtspServer.createCameraStream( packetPublisher, "streamId2", getCodecs(), "" );
    rtspServer.deleteCameraStream( "streamId2" );

    rtspServer.createCameraStream( packetPublisher, "streamId3", getCodecs(), "" );
    rtspServer.deleteCameraStream( "streamId3" );

    rtspServer.createCameraStream( packetPublisher, "streamId4", getCodecs(), "" );
    rtspServer.deleteCameraStream( "streamId4" );
}

TEST_F( RtspServerControllerTests, createAndDeleteSequentiallyOneStream ) {  // 1.5
    MockIPacketPublisher packetPublisher;
    MockIEventPublisher eventPublisher;

    EXPECT_CALL( eventPublisher, sendEvent( EventBase( "streamId1", EventType::STREAM_CREATED ) ) );
    EXPECT_CALL( eventPublisher, sendEvent( EventBase( "streamId1", EventType::STREAM_DELETED ) ) );

    RtspServerController rtspServer( system_utils::getUniqueFreeLocalPort(), PacketCountLimit, eventPublisher );
    rtspServer.start();
    ASSERT_TRUE( rtspServer.waitForStart( WaitTimeoutSec ) );

    rtspServer.createCameraStream( packetPublisher, "streamId1", getCodecs(), "" );
    rtspServer.createCameraStream( packetPublisher, "streamId1", getCodecs(), "" );
    rtspServer.createCameraStream( packetPublisher, "streamId1", getCodecs(), "" );
    rtspServer.createCameraStream( packetPublisher, "streamId1", getCodecs(), "" );

    rtspServer.deleteCameraStream( "streamId1" );
    rtspServer.deleteCameraStream( "streamId1" );
    rtspServer.deleteCameraStream( "streamId1" );
    rtspServer.deleteCameraStream( "streamId1" );
}

TEST_F( RtspServerControllerTests, createAndDeleteSequentiallySeveralStreams ) {  // 1.6
    MockIPacketPublisher packetPublisher;
    MockIEventPublisher eventPublisher;

    EXPECT_CALL( eventPublisher, sendEvent( EventBase( "streamId1", EventType::STREAM_CREATED ) ) );
    EXPECT_CALL( eventPublisher, sendEvent( EventBase( "streamId2", EventType::STREAM_CREATED ) ) );
    EXPECT_CALL( eventPublisher, sendEvent( EventBase( "streamId3", EventType::STREAM_CREATED ) ) );

    EXPECT_CALL( eventPublisher, sendEvent( EventBase( "streamId1", EventType::STREAM_DELETED ) ) );
    EXPECT_CALL( eventPublisher, sendEvent( EventBase( "streamId2", EventType::STREAM_DELETED ) ) );
    EXPECT_CALL( eventPublisher, sendEvent( EventBase( "streamId3", EventType::STREAM_DELETED ) ) );

    RtspServerController rtspServer( system_utils::getUniqueFreeLocalPort(), PacketCountLimit, eventPublisher );
    rtspServer.start();
    ASSERT_TRUE( rtspServer.waitForStart( WaitTimeoutSec ) );

    rtspServer.createCameraStream( packetPublisher, "streamId1", getCodecs(), "" );
    rtspServer.createCameraStream( packetPublisher, "streamId1", getCodecs(), "" );
    rtspServer.createCameraStream( packetPublisher, "streamId1", getCodecs(), "" );
    rtspServer.createCameraStream( packetPublisher, "streamId1", getCodecs(), "" );

    rtspServer.deleteCameraStream( "streamId1" );
    rtspServer.deleteCameraStream( "streamId1" );
    rtspServer.deleteCameraStream( "streamId1" );
    rtspServer.deleteCameraStream( "streamId1" );

    rtspServer.createCameraStream( packetPublisher, "streamId2", getCodecs(), "" );
    rtspServer.createCameraStream( packetPublisher, "streamId2", getCodecs(), "" );
    rtspServer.createCameraStream( packetPublisher, "streamId2", getCodecs(), "" );
    rtspServer.createCameraStream( packetPublisher, "streamId2", getCodecs(), "" );

    rtspServer.deleteCameraStream( "streamId2" );
    rtspServer.deleteCameraStream( "streamId2" );
    rtspServer.deleteCameraStream( "streamId2" );
    rtspServer.deleteCameraStream( "streamId2" );

    rtspServer.createCameraStream( packetPublisher, "streamId3", getCodecs(), "" );
    rtspServer.createCameraStream( packetPublisher, "streamId3", getCodecs(), "" );
    rtspServer.createCameraStream( packetPublisher, "streamId3", getCodecs(), "" );
    rtspServer.createCameraStream( packetPublisher, "streamId3", getCodecs(), "" );

    rtspServer.deleteCameraStream( "streamId3" );
    rtspServer.deleteCameraStream( "streamId3" );
    rtspServer.deleteCameraStream( "streamId3" );
    rtspServer.deleteCameraStream( "streamId3" );
}

TEST_F( RtspServerControllerTests, WaitForServerStartAndStopServer ) {  // 1.7
    MockIPacketPublisher packetPublisher;
    EventPublisher< EventBase > eventPublisher;

    RtspServerController rtspServer( system_utils::getUniqueFreeLocalPort(), PacketCountLimit, eventPublisher );

    rtspServer.start();
    ASSERT_TRUE( rtspServer.waitForStart( WaitTimeoutSec ) );
    rtspServer.stop();

    rtspServer.start();
    ASSERT_TRUE( rtspServer.waitForStart( WaitTimeoutSec ) );
    rtspServer.stop();

    rtspServer.start();
    ASSERT_TRUE( rtspServer.waitForStart( WaitTimeoutSec ) );
    rtspServer.stop();
}

TEST_F( RtspServerControllerTests, startStopServerWithStreamCreating ) {  // 1.8
    MockIPacketPublisher packetPublisher;
    EventPublisher< EventBase > eventPublisher;

    RtspServerController rtspServer( system_utils::getUniqueFreeLocalPort(), PacketCountLimit, eventPublisher );

    rtspServer.start();
    ASSERT_TRUE( rtspServer.waitForStart( WaitTimeoutSec ) );
    rtspServer.createCameraStream( packetPublisher, "streamId1", getCodecs(), "" );
    rtspServer.deleteCameraStream( "streamId1" );
    rtspServer.stop();
    EXPECT_EQ( rtspServer.getStreamQuantity(), 0 );

    rtspServer.start();
    ASSERT_TRUE( rtspServer.waitForStart( WaitTimeoutSec ) );
    rtspServer.createCameraStream( packetPublisher, "streamId1", getCodecs(), "" );
    rtspServer.deleteCameraStream( "streamId1" );
    rtspServer.stop();
    EXPECT_EQ( rtspServer.getStreamQuantity(), 0 );

    rtspServer.start();
    ASSERT_TRUE( rtspServer.waitForStart( WaitTimeoutSec ) );
    rtspServer.createCameraStream( packetPublisher, "streamId1", getCodecs(), "" );
    rtspServer.deleteCameraStream( "streamId1" );
    rtspServer.stop();
    EXPECT_EQ( rtspServer.getStreamQuantity(), 0 );
}

TEST_F( RtspServerControllerTests, createAndStopServerAfterDelay ) {  // 1.9
    MockIPacketPublisher packetPublisher;
    EventPublisher< EventBase > eventPublisher;

    RtspServerController rtspServer( system_utils::getUniqueFreeLocalPort(), PacketCountLimit, eventPublisher );
    rtspServer.start();
    ASSERT_TRUE( rtspServer.waitForStart( WaitTimeoutSec ) );

    rtspServer.createCameraStream( packetPublisher, "streamId1", getCodecs(), "" );
    rtspServer.createCameraStream( packetPublisher, "streamId2", getCodecs(), "" );
    rtspServer.createCameraStream( packetPublisher, "streamId3", getCodecs(), "" );
    rtspServer.createCameraStream( packetPublisher, "streamId4", getCodecs(), "" );
    rtspServer.createCameraStream( packetPublisher, "streamId5", getCodecs(), "" );
    std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
    EXPECT_EQ( rtspServer.getStreamQuantity(), 5 );
    rtspServer.stop();
    EXPECT_EQ( rtspServer.getStreamQuantity(), 0 );
}

}  // namespace UT
}  // namespace Live555
}  // namespace Cameras
}  // namespace HubPlatform
