#include "cameras/tests/http_file_server.h"

#include "cameras/tests/samples.h"

#include "cameras/input_output/camera_input.h"
#include "cameras/input_output/camera_input_events_listener_interface.h"
#include "cameras/input_output/camera_input_options.h"
#include "cameras/input_output/camera_archiver.h"
#include "cameras/input_output/camera_output.h"
#include "cameras/packet/packet_publisher.h"
#include "test_utils/temporary_folder_wrapper.h"

#include "common_utils/cpp14.h"

#include <gtest/gtest.h>

using namespace ::testing;

namespace HubPlatform {
namespace UT {

class FakeCameraInputEventsListener : public ICameraInputEventsListener {
public:
    FakeCameraInputEventsListener( std::function< void( int ) > readingStoppedCallback,
                                   std::function< void() > packetsDelayCallback,
                                   std::function< void() > accumulatedDelayCallback )
    : m_readingStoppedCallback( std::move( readingStoppedCallback ) )
    , m_packetsDelayCallback( std::move( packetsDelayCallback ) )
    , m_accumulatedDelayCallback( std::move( accumulatedDelayCallback ) ) {}

    void onInputReadingStopped( int errorCode ) override {
        if( m_readingStoppedCallback )
            m_readingStoppedCallback( errorCode );
    };
    void onMaxPacketsDelayViolated() override {
        if( m_packetsDelayCallback )
            m_packetsDelayCallback();
    };
    void onAccumulatedDelayThresholdReached() override {
        if( m_accumulatedDelayCallback )
            m_accumulatedDelayCallback();
    };

private:
    std::function< void( int ) > m_readingStoppedCallback;
    std::function< void() > m_packetsDelayCallback;
    std::function< void() > m_accumulatedDelayCallback;
};

class CameraInputTestSuite : public Test {
public:
    void SetUp() override {
        CameraInputOptions::setMaxDelayBetweenPacketsMsec( 100 );
        CameraInputOptions::setPacketProcessingDelayMsec( 50 );
        CameraInputOptions::setAccumulatedDelayThresholdMsec( 300 );

        packetHandler = std::make_shared< PacketHandler >();
        cameraInput = std::make_unique< CameraInput >( packetHandler, PathVideo20secondsMp4 );
    }

    bool checkAccumulatedDelayDetected() {
        std::promise< void > promise;
        std::future< void > future = promise.get_future();
        FakeCameraInputEventsListener eventsListener( nullptr, nullptr, [&promise]() { promise.set_value(); } );

        cameraInput->setEventsListener( &eventsListener );
        const bool startedSuccessfully = cameraInput->start();
        EXPECT_TRUE( startedSuccessfully );

        bool delayDetected = false;
        if( startedSuccessfully ) {
            const std::future_status status = future.wait_for( std::chrono::seconds( 2 ) );
            delayDetected = status == std::future_status::ready;
        }

        cameraInput->setEventsListener( nullptr );
        return delayDetected;
    }

    void setDefaultDelaysForInput() {
        cameraInput->m_accumulatedDelayThreshold = std::chrono::milliseconds( DefaultAccumulatedDelayThresholdMsec );
        cameraInput->m_packetsProcessingDelay = std::chrono::milliseconds( 0 );
    }

    PacketHandlerSharedPtr packetHandler;
    CameraInputUniquePtr cameraInput;
};

TEST_F( CameraInputTestSuite, accumulatedDelayDetected ) {
    EXPECT_TRUE( checkAccumulatedDelayDetected() );
    cameraInput.reset();
}

TEST_F( CameraInputTestSuite, cameraInputCanBeRestartedAfterDelayDetection ) {
    const bool delayDetected = checkAccumulatedDelayDetected();
    EXPECT_TRUE( delayDetected );
    if( delayDetected ) {
        EXPECT_TRUE( cameraInput->start() );
    }
    cameraInput.reset();
}

TEST_F( CameraInputTestSuite, accumulatedDelayDetectedMultipleTimes ) {
    for( int i = 0; i < 3; ++i ) {
        const bool delayDetected = checkAccumulatedDelayDetected();
        EXPECT_TRUE( delayDetected );
        if( !delayDetected ) {
            break;
        }
    }
    cameraInput.reset();
}

TEST_F( CameraInputTestSuite, successfulStreamingAfter ) {
    const bool delayDetected = checkAccumulatedDelayDetected();
    EXPECT_TRUE( delayDetected );
    if( delayDetected ) {
        setDefaultDelaysForInput();
        const bool delayDetected = checkAccumulatedDelayDetected();
        EXPECT_FALSE( delayDetected );
    }
    cameraInput.reset();
}

}  // namespace UT
}  // namespace HubPlatform
