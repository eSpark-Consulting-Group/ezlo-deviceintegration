#include "cameras/defs.h"
#include "cameras/packet/packet_publisher.h"
#include "cameras/camera.h"
#include "cameras/live_rtsp_stream_status_notifier_interface.h"

#include "cameras/input_output/camera_input.h"
#include "cameras/input_output/camera_input_options.h"
#include "cameras/postponed/postponed_method_manager_interface.h"
#include "cameras/local_stream/live_streamer_interface.h"
#include "cameras/cloud/cloud_stream_saver_interface.h"
#include "cameras/cloud/stream_particle_recorder_interface.h"
#include "cameras/snapshot/snapshot_manager_interface.h"

#include "json_utils/json_utils.h"
#include "common_utils/cpp14.h"

#include <exception>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace ::testing;

namespace HubPlatform {
namespace UT {

class MockCameraInput : public ICameraInput {
public:
    MOCK_METHOD1( start, bool( RtspTransport rtspTransport ) );
    MOCK_METHOD0( pause, void() );
    MOCK_METHOD0( run, void() );
    MOCK_METHOD0( stop, void() );
    MOCK_CONST_METHOD0( isRunning, bool() );
    MOCK_CONST_METHOD0( getVideoStream, const AVStream*() );
    MOCK_CONST_METHOD0( getAudioStream, const AVStream*() );
    MOCK_CONST_METHOD0( getSdp, const std::string&() );
    MOCK_CONST_METHOD0( getVideoIndex, int() );
    MOCK_CONST_METHOD0( getInputCodecContext, const AVCodecContext*() );
    MOCK_CONST_METHOD0( getPacketHandler, const PacketHandlerSharedPtr&() );
    MOCK_METHOD1( setEventsListener, void( ICameraInputEventsListener* ) );
    MOCK_CONST_METHOD0( getError, std::string() );
};

class MockPostponedMethodManager : public IPostponedMethodManager {
public:
    MOCK_METHOD0( runAllMethods, void() );
    MOCK_METHOD2( addMethod, void( MethodType, PostponedMethod ) );
    MOCK_METHOD1( removeMethod, void( MethodType ) );
};

class MockLiveStreamer : public ILiveStreamer {
public:
    MOCK_METHOD0( createCameraStream, void() );
    MOCK_METHOD0( deleteCameraStream, void() );
};

class MockCloudStreamSaver : public ICloudStreamSaver {
public:
    MOCK_CONST_METHOD0( isSavingCreated, bool() );
    MOCK_CONST_METHOD0( isStarted, bool() );
    MOCK_METHOD5( createRecording,
                  void( int segmentDurationSec,
                        int64_t secondsBeforeStart,
                        int segmentsLimit,
                        const std::string&,
                        SaveFileCallback saveFile ) );
    MOCK_METHOD0( deleteRecording, void() );
    MOCK_METHOD2( startRecording, void( const Json& metaData, SaveFileCallback saveFile ) );
    MOCK_METHOD0( startRecording, void() );
    MOCK_METHOD0( stopRecording, void() );
};

class MockStreamParticleRecorder : public IStreamParticleRecorder {
public:
    MOCK_CONST_METHOD0( isRecordingCreated, bool() );
    MOCK_CONST_METHOD0( isMakingRecord, bool() );
    MOCK_METHOD2( createRecording, void( const StreamParticleSettings&, SaveFileCallback saveFile ) );
    MOCK_METHOD0( deleteRecording, void() );
    MOCK_METHOD1( makeRecord, void( const Json& metaData ) );
    MOCK_METHOD0( makeRecord, void() );
    MOCK_METHOD0( stopMakingRecord, void() );
};

class MockSnapshotManager : public ISnapshotManager {
public:
    MOCK_METHOD4( makeSnapshotForCloud,
                  void( const Json& metaData,
                        SaveFileCallback saveFile,
                        const std::string& snapshotId,
                        Response responseCallback ) );
    MOCK_METHOD3( makeSnapshotForUI,
                  void( std::shared_ptr< Resolution > resolution,
                        const std::string& requestId,
                        Response responseCallback ) );
};

class MockLiveRTSPStreamStatusNotifier : public ILiveRTSPStreamStatusNotifier {
public:
    MOCK_METHOD2( onRTSPStreamStarted, void( const StreamId&, const LiveRTSPStreamLink& ) );
    MOCK_METHOD1( onRTSPStreamStopped, void( const StreamId& ) );
    MOCK_METHOD2( onRTSPStreamCreationFailed, void( const StreamId&, const std::string& ) );
    MOCK_METHOD2( onRTSPStreamError, void( const StreamId&, StreamError ) );
};

class CameraTestSuite : public Test {
public:
    void SetUp() override {
        createCamera();
    }

    Camera& getCamera() {
        return *camera;
    }

    void createCamera() {
        CameraInputOptions::setMaxDelayBetweenPacketsMsec( 100 );
        CameraInputOptions::setPacketProcessingDelayMsec( 0 );
        CameraInputOptions::setAccumulatedDelayThresholdMsec( DefaultAccumulatedDelayThresholdMsec );
        auto cameraInput = std::make_unique< MockCameraInput >();
        auto postponedMethodManager = std::make_unique< MockPostponedMethodManager >();
        auto liveStreamer = std::make_unique< MockLiveStreamer >();
        auto cloudStreamSaver = std::make_unique< MockCloudStreamSaver >();
        auto particleRecorder = std::make_unique< MockStreamParticleRecorder >();
        auto snapshotManager = std::make_unique< MockSnapshotManager >();
        mockLiveRTSPStreamStatusNotifier = std::make_unique< MockLiveRTSPStreamStatusNotifier >();

        EXPECT_CALL( *cameraInput, setEventsListener( _ ) ).Times( 1 );
        camera = std::make_unique< Camera >( "streamId",
                                             "ip_address",
                                             std::move( cameraInput ),
                                             std::move( postponedMethodManager ),
                                             std::move( liveStreamer ),
                                             std::move( cloudStreamSaver ),
                                             std::move( particleRecorder ),
                                             std::move( snapshotManager ),
                                             *mockLiveRTSPStreamStatusNotifier );
    }

    void removeCamera() {
        camera.reset();
    }

    void setStartTimeout( const std::chrono::seconds& timeout ) {
        camera->m_startStreamTriesSleepTime = timeout;
    }

    MockCameraInput& getMockCameraInput() {
        return dynamic_cast< MockCameraInput& >( *camera->m_cameraInput );
    }

    MockPostponedMethodManager& getMockPostponedMethodManager() {
        return dynamic_cast< MockPostponedMethodManager& >( *camera->m_postponedMethodManager );
    }

    MockLiveStreamer& getMockLiveStreamer() {
        return dynamic_cast< MockLiveStreamer& >( *camera->m_liveStreamer );
    }

    MockCloudStreamSaver& getMockCloudStreamSaver() {
        return dynamic_cast< MockCloudStreamSaver& >( *camera->m_cloudStreamSaver );
    }

    MockStreamParticleRecorder& getMockStreamParticleRecorder() {
        return dynamic_cast< MockStreamParticleRecorder& >( *camera->m_particleRecorder );
    }

    MockSnapshotManager& getMockSnapshotManager() {
        return dynamic_cast< MockSnapshotManager& >( *camera->m_snapshotManager );
    }

    MockLiveRTSPStreamStatusNotifier& getMockLiveRTSPStreamStatusNotifier() {
        return *mockLiveRTSPStreamStatusNotifier;
    }

    using LiveRTSPStreamStatus = Camera::LiveRTSPStreamStatus;
    LiveRTSPStreamStatus& getLiveRTSPStreamStatus() {
        return camera->m_liveRTSPStreamStatus;
    }

    void triggerStreamReadingStopCallback() {
        camera->onStreamReadingStop();
    }

    void onStreamReadingStopCallback() {
        camera->m_surpriseTriggeringBlocker = true;
        camera->m_startingCondVar.notify_all();
    }

    void clearConditionalVariable() {
        std::unique_lock< std::mutex > lock( camera->m_startingMutex );
        camera->m_startingCondVar.wait_for( lock, std::chrono::milliseconds( 10 ) );
        camera->m_surpriseTriggeringBlocker = false;
    }

    static StreamParticleSettings createPreviewRecorderSettings() {
        StreamParticleSettings settings;
        settings.preRecordDuration = std::chrono::seconds( 2 );
        settings.recordDuration = std::chrono::seconds( 3 );
        settings.resolutionDivisor = 4;
        return settings;
    }

private:
    std::unique_ptr< Camera > camera;
    std::unique_ptr< MockLiveRTSPStreamStatusNotifier > mockLiveRTSPStreamStatusNotifier;
};

TEST_F( CameraTestSuite, startStream ) {
    EXPECT_CALL( getMockCameraInput(), start ).WillOnce( Return( true ) );
    EXPECT_CALL( getMockCameraInput(), isRunning ).Times( 2 ).WillRepeatedly( Return( true ) );
    EXPECT_CALL( getMockPostponedMethodManager(), runAllMethods ).Times( 1 );
    EXPECT_CALL( getMockCloudStreamSaver(), deleteRecording ).Times( 1 );
    EXPECT_CALL( getMockStreamParticleRecorder(), deleteRecording ).Times( 1 );
    EXPECT_CALL( getMockStreamParticleRecorder(), isRecordingCreated ).WillRepeatedly( Return( false ) );
    EXPECT_CALL( getMockCloudStreamSaver(), isSavingCreated ).Times( 1 );
    EXPECT_NO_THROW( getCamera().startStream() );
    EXPECT_NO_THROW( getCamera().startStream() );
    EXPECT_NO_THROW( getCamera().startStream() );
    std::this_thread::yield();
    std::this_thread::sleep_for( std::chrono::milliseconds( 250 ) );
    removeCamera();
}

TEST_F( CameraTestSuite, restartStream ) {
    EXPECT_CALL( getMockCameraInput(), start ).Times( 3 ).WillRepeatedly( Return( true ) );
    EXPECT_CALL( getMockCameraInput(), isRunning ).Times( 4 ).WillRepeatedly( Return( true ) );
    EXPECT_CALL( getMockPostponedMethodManager(), runAllMethods ).Times( 3 );
    EXPECT_CALL( getMockCloudStreamSaver(), deleteRecording ).Times( 1 );
    EXPECT_CALL( getMockCloudStreamSaver(), isSavingCreated ).Times( 3 ).WillRepeatedly( Return( false ) );
    EXPECT_CALL( getMockStreamParticleRecorder(), deleteRecording ).Times( 1 );
    EXPECT_CALL( getMockStreamParticleRecorder(), isRecordingCreated ).WillRepeatedly( Return( false ) );
    EXPECT_NO_THROW( getCamera().startStream() );
    std::this_thread::yield();
    std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
    onStreamReadingStopCallback();
    std::this_thread::yield();
    std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
    onStreamReadingStopCallback();
    std::this_thread::yield();
    std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
    removeCamera();
}

TEST_F( CameraTestSuite, createCamera ) {
    EXPECT_CALL( getMockCameraInput(), start ).Times( 0 );
    EXPECT_CALL( getMockCameraInput(), isRunning ).Times( 1 ).WillRepeatedly( Return( false ) );
    EXPECT_CALL( getMockCloudStreamSaver(), deleteRecording ).Times( 0 );
    EXPECT_CALL( getMockStreamParticleRecorder(), deleteRecording ).Times( 1 );
    EXPECT_CALL( getMockLiveStreamer(), deleteCameraStream ).Times( 0 );
    EXPECT_CALL( getMockPostponedMethodManager(), removeMethod( IPostponedMethodManager::MethodType::StartLiveRtsp ) )
        .Times( 0 );
    EXPECT_CALL( getMockPostponedMethodManager(),
                 removeMethod( IPostponedMethodManager::MethodType::CreateSavingToCloud ) )
        .Times( 1 );
    EXPECT_CALL( getMockPostponedMethodManager(),
                 removeMethod( IPostponedMethodManager::MethodType::StartSavingToCloud ) )
        .Times( 0 );
    EXPECT_CALL( getMockPostponedMethodManager(), runAllMethods ).Times( 0 );
    std::this_thread::yield();
    removeCamera();
}

TEST_F( CameraTestSuite, createAndDeleteLiveStream ) {
    EXPECT_CALL( getMockCameraInput(), start ).Times( 0 );
    EXPECT_CALL( getMockCameraInput(), isRunning ).Times( 2 ).WillRepeatedly( Return( true ) );
    EXPECT_CALL( getMockCloudStreamSaver(), deleteRecording ).Times( 1 );
    EXPECT_CALL( getMockStreamParticleRecorder(), deleteRecording ).Times( 1 );
    EXPECT_CALL( getMockLiveStreamer(), createCameraStream ).Times( 1 );
    EXPECT_CALL( getMockLiveStreamer(), deleteCameraStream ).Times( 0 );
    EXPECT_CALL( getMockLiveRTSPStreamStatusNotifier(), onRTSPStreamStarted );

    EXPECT_CALL( getMockPostponedMethodManager(), removeMethod( IPostponedMethodManager::MethodType::StartLiveRtsp ) )
        .Times( 0 );
    EXPECT_CALL( getMockPostponedMethodManager(),
                 removeMethod( IPostponedMethodManager::MethodType::CreateSavingToCloud ) )
        .Times( 0 );
    EXPECT_CALL( getMockPostponedMethodManager(),
                 removeMethod( IPostponedMethodManager::MethodType::StartSavingToCloud ) )
        .Times( 0 );
    EXPECT_CALL( getMockPostponedMethodManager(), runAllMethods ).Times( 0 );
    auto& camera = getCamera();
    camera.createLiveRtspStream();
    std::this_thread::yield();
    removeCamera();
}

TEST_F( CameraTestSuite, checkStatusChangeEventsLiveStream ) {
    EXPECT_CALL( getMockCameraInput(), isRunning ).WillRepeatedly( Return( true ) );

    EXPECT_CALL( getMockLiveRTSPStreamStatusNotifier(), onRTSPStreamStarted ).Times( 2 );
    EXPECT_CALL( getMockLiveRTSPStreamStatusNotifier(), onRTSPStreamStopped ).Times( 2 );

    EXPECT_CALL( getMockLiveStreamer(), createCameraStream ).WillRepeatedly( Return() );
    EXPECT_CALL( getMockLiveStreamer(), deleteCameraStream ).WillRepeatedly( Return() );
    EXPECT_CALL( getMockCloudStreamSaver(), deleteRecording ).WillRepeatedly( Return() );
    EXPECT_CALL( getMockStreamParticleRecorder(), deleteRecording ).WillRepeatedly( Return() );

    auto& camera = getCamera();
    camera.createLiveRtspStream();
    camera.createLiveRtspStream();
    camera.deleteLiveRtspStream();
    camera.createLiveRtspStream();
    camera.deleteLiveRtspStream();

    std::this_thread::yield();
    removeCamera();
}

TEST_F( CameraTestSuite, startAndStopStreamSaving ) {
    EXPECT_CALL( getMockCameraInput(), start ).Times( 0 );
    EXPECT_CALL( getMockCameraInput(), isRunning ).Times( 5 ).WillRepeatedly( Return( true ) );
    EXPECT_CALL( getMockCloudStreamSaver(), isStarted ).Times( 1 ).WillRepeatedly( Return( false ) );
    EXPECT_CALL( getMockCloudStreamSaver(), createRecording ).Times( 1 );
    EXPECT_CALL( getMockCloudStreamSaver(), deleteRecording ).Times( 2 );
    EXPECT_CALL( getMockCloudStreamSaver(), stopRecording ).Times( 1 );
    EXPECT_CALL( getMockCloudStreamSaver(), startRecording( _, _ ) ).Times( 1 );
    EXPECT_CALL( getMockCloudStreamSaver(), isSavingCreated ).WillOnce( Return( false ) );
    EXPECT_CALL( getMockLiveStreamer(), createCameraStream ).Times( 0 );
    EXPECT_CALL( getMockLiveStreamer(), deleteCameraStream ).Times( 0 );
    EXPECT_CALL( getMockStreamParticleRecorder(), deleteRecording ).WillRepeatedly( Return() );
    EXPECT_CALL( getMockPostponedMethodManager(), removeMethod( IPostponedMethodManager::MethodType::StartLiveRtsp ) )
        .Times( 0 );
    EXPECT_CALL( getMockPostponedMethodManager(),
                 removeMethod( IPostponedMethodManager::MethodType::CreateSavingToCloud ) )
        .Times( 0 );
    EXPECT_CALL( getMockPostponedMethodManager(),
                 removeMethod( IPostponedMethodManager::MethodType::StartSavingToCloud ) )
        .Times( 0 );
    EXPECT_CALL( getMockPostponedMethodManager(), runAllMethods ).Times( 0 );
    auto& camera = getCamera();
    camera.createStreamSavingToCloud(
        30, 5, 0, "", []( const FilePath&, const ResultCallback&, const FileRemoverSharedPtr&, const Json& ) {} );
    std::this_thread::yield();
    EXPECT_CALL( getMockCloudStreamSaver(), isSavingCreated ).Times( 2 ).WillRepeatedly( Return( true ) );
    camera.startStreamSavingToCloud(
        {}, []( const FilePath&, const ResultCallback&, const FileRemoverSharedPtr&, const Json& ) {} );
    camera.stopStreamSavingToCloud();
    camera.deleteStreamSavingToCloud();
    std::this_thread::yield();
    removeCamera();
}

TEST_F( CameraTestSuite, startStreamSavingConsecutivelyMultipleTimes ) {
    EXPECT_CALL( getMockCameraInput(), start ).Times( 0 );
    EXPECT_CALL( getMockCameraInput(), isRunning ).Times( 3 ).WillRepeatedly( Return( true ) );

    EXPECT_CALL( getMockCloudStreamSaver(), isStarted ).WillOnce( Return( false ) );
    EXPECT_CALL( getMockCloudStreamSaver(), isSavingCreated ).WillOnce( Return( false ) );
    EXPECT_CALL( getMockCloudStreamSaver(), createRecording ).Times( 1 );
    EXPECT_CALL( getMockCloudStreamSaver(), deleteRecording ).Times( 1 );
    EXPECT_CALL( getMockCloudStreamSaver(), stopRecording ).Times( 0 );
    EXPECT_CALL( getMockCloudStreamSaver(), startRecording( _, _ ) ).Times( 1 );

    EXPECT_CALL( getMockPostponedMethodManager(), addMethod( _, _ ) ).Times( 0 );
    EXPECT_CALL( getMockPostponedMethodManager(), removeMethod( _ ) ).Times( 0 );

    EXPECT_CALL( getMockStreamParticleRecorder(), deleteRecording ).WillRepeatedly( Return() );

    auto& camera = getCamera();
    auto saveFileCallback = []( const FilePath&, const ResultCallback&, const FileRemoverSharedPtr&, const Json& ) {};
    camera.createStreamSavingToCloud( 30, 5, 0, "", saveFileCallback );
    std::this_thread::yield();
    EXPECT_CALL( getMockCloudStreamSaver(), isSavingCreated ).Times( 3 ).WillRepeatedly( Return( true ) );

    camera.startStreamSavingToCloud( {}, saveFileCallback );
    EXPECT_CALL( getMockCloudStreamSaver(), isStarted ).Times( 2 ).WillRepeatedly( Return( true ) );
    camera.startStreamSavingToCloud( {}, saveFileCallback );
    camera.startStreamSavingToCloud( {}, saveFileCallback );
    std::this_thread::yield();
    removeCamera();
}

TEST_F( CameraTestSuite, startStreamSavingConsecutivelyMultipleTimesDifferentParams ) {
    EXPECT_CALL( getMockCameraInput(), start ).Times( 0 );
    EXPECT_CALL( getMockCameraInput(), isRunning ).Times( 3 ).WillRepeatedly( Return( true ) );

    EXPECT_CALL( getMockCloudStreamSaver(), isStarted ).WillOnce( Return( false ) );
    EXPECT_CALL( getMockCloudStreamSaver(), isSavingCreated ).WillOnce( Return( false ) );
    EXPECT_CALL( getMockCloudStreamSaver(), createRecording ).Times( 1 );
    EXPECT_CALL( getMockCloudStreamSaver(), deleteRecording ).Times( 0 );
    EXPECT_CALL( getMockCloudStreamSaver(), stopRecording ).Times( 0 );
    EXPECT_CALL( getMockCloudStreamSaver(), startRecording( _, _ ) ).Times( 1 );

    EXPECT_CALL( getMockPostponedMethodManager(), addMethod( _, _ ) ).Times( 0 );
    EXPECT_CALL( getMockPostponedMethodManager(), removeMethod( _ ) ).Times( 0 );

    EXPECT_CALL( getMockStreamParticleRecorder(), deleteRecording ).WillRepeatedly( Return() );

    auto& camera = getCamera();

    auto saveFileCallback = []( const FilePath&, const ResultCallback&, const FileRemoverSharedPtr&, const Json& ) {};
    camera.createStreamSavingToCloud( 30, 5, 0, "", saveFileCallback );
    std::this_thread::yield();
    EXPECT_CALL( getMockCloudStreamSaver(), isSavingCreated ).Times( 3 ).WillRepeatedly( Return( true ) );
    EXPECT_CALL( getMockCloudStreamSaver(), deleteRecording ).WillRepeatedly( Return() );
    EXPECT_CALL( getMockStreamParticleRecorder(), deleteRecording ).WillRepeatedly( Return() );

    camera.startStreamSavingToCloud( Json{}, saveFileCallback );
    EXPECT_CALL( getMockCloudStreamSaver(), isStarted ).Times( 2 ).WillRepeatedly( Return( true ) );
    camera.startStreamSavingToCloud( Json{ { "meta", "data" } }, saveFileCallback );
    camera.startStreamSavingToCloud( Json{ { "meta", "data" }, { "data", "meta" } }, saveFileCallback );
    EXPECT_CALL( getMockCloudStreamSaver(), deleteRecording ).WillRepeatedly( Return() );
    EXPECT_CALL( getMockStreamParticleRecorder(), deleteRecording ).WillRepeatedly( Return() );
    std::this_thread::yield();
    removeCamera();
}

TEST_F( CameraTestSuite, startAndStopPreviewRecording ) {
    EXPECT_CALL( getMockCameraInput(), start ).Times( 0 );
    EXPECT_CALL( getMockCameraInput(), isRunning ).Times( 2 ).WillRepeatedly( Return( true ) );
    EXPECT_CALL( getMockCloudStreamSaver(), deleteRecording ).WillRepeatedly( Return() );
    EXPECT_CALL( getMockStreamParticleRecorder(), createRecording ).Times( 1 );
    EXPECT_CALL( getMockStreamParticleRecorder(), deleteRecording ).Times( 0 );
    EXPECT_CALL( getMockStreamParticleRecorder(), makeRecord( _ ) ).Times( 1 );
    EXPECT_CALL( getMockStreamParticleRecorder(), isMakingRecord ).WillOnce( Return( false ) );
    EXPECT_CALL( getMockStreamParticleRecorder(), isRecordingCreated ).WillOnce( Return( false ) );

    auto& camera = getCamera();

    auto saveFileCallback = []( const FilePath&, const ResultCallback&, const FileRemoverSharedPtr&, const Json& ) {};
    camera.createStreamParticleRecording( createPreviewRecorderSettings(), saveFileCallback );
    std::this_thread::yield();

    EXPECT_CALL( getMockStreamParticleRecorder(), isRecordingCreated ).WillOnce( Return( true ) );
    EXPECT_CALL( getMockStreamParticleRecorder(), deleteRecording ).WillRepeatedly( Return() );
    camera.recordStreamParticle( Json{} );
    camera.deleteStreamParticleRecording();

    std::this_thread::yield();
    removeCamera();
}

TEST_F( CameraTestSuite, particleRecordingCreatedOnlyOnce ) {
    EXPECT_CALL( getMockCameraInput(), start ).Times( 0 );
    EXPECT_CALL( getMockCameraInput(), isRunning ).WillRepeatedly( Return( true ) );
    EXPECT_CALL( getMockCloudStreamSaver(), deleteRecording ).WillRepeatedly( Return() );
    EXPECT_CALL( getMockStreamParticleRecorder(), createRecording ).Times( 1 );
    EXPECT_CALL( getMockStreamParticleRecorder(), deleteRecording ).Times( 1 );
    EXPECT_CALL( getMockStreamParticleRecorder(), makeRecord( _ ) ).Times( 0 );
    EXPECT_CALL( getMockStreamParticleRecorder(), isRecordingCreated ).WillOnce( Return( false ) );

    auto& camera = getCamera();

    auto saveFileCallback = []( const FilePath&, const ResultCallback&, const FileRemoverSharedPtr&, const Json& ) {};
    const StreamParticleSettings recorderSettings = createPreviewRecorderSettings();
    camera.createStreamParticleRecording( recorderSettings, saveFileCallback );
    std::this_thread::yield();

    EXPECT_CALL( getMockStreamParticleRecorder(), isRecordingCreated ).Times( 2 ).WillRepeatedly( Return( true ) );
    camera.createStreamParticleRecording( recorderSettings, saveFileCallback );
    camera.createStreamParticleRecording( recorderSettings, saveFileCallback );

    std::this_thread::yield();
    removeCamera();
}

TEST_F( CameraTestSuite, createPostponedLiveStream1 ) {
    EXPECT_CALL( getMockCameraInput(), start ).Times( 0 );
    EXPECT_CALL( getMockCameraInput(), isRunning ).Times( 2 ).WillRepeatedly( Return( false ) );
    EXPECT_CALL( getMockCloudStreamSaver(), createRecording ).Times( 0 );
    EXPECT_CALL( getMockCloudStreamSaver(), deleteRecording ).Times( 0 );
    EXPECT_CALL( getMockCloudStreamSaver(), isSavingCreated ).Times( 0 );
    EXPECT_CALL( getMockLiveStreamer(), deleteCameraStream ).Times( 0 );
    EXPECT_CALL( getMockStreamParticleRecorder(), deleteRecording ).WillRepeatedly( Return() );
    EXPECT_CALL( getMockPostponedMethodManager(), addMethod( IPostponedMethodManager::MethodType::StartLiveRtsp, _ ) )
        .Times( 1 );
    EXPECT_CALL( getMockPostponedMethodManager(), removeMethod( IPostponedMethodManager::MethodType::StartLiveRtsp ) )
        .Times( 0 );
    EXPECT_CALL( getMockPostponedMethodManager(),
                 removeMethod( IPostponedMethodManager::MethodType::CreateSavingToCloud ) )
        .Times( 1 );
    EXPECT_CALL( getMockPostponedMethodManager(),
                 removeMethod( IPostponedMethodManager::MethodType::StartSavingToCloud ) )
        .Times( 0 );
    EXPECT_CALL( getMockPostponedMethodManager(), runAllMethods ).Times( 0 );
    auto& camera = getCamera();
    camera.createLiveRtspStream();
    std::this_thread::yield();
    removeCamera();
}

TEST_F( CameraTestSuite, createPostponedLiveStream2 ) {
    EXPECT_CALL( getMockCameraInput(), start ).WillOnce( Return( true ) );
    EXPECT_CALL( getMockCameraInput(), isRunning ).WillRepeatedly( Return( false ) );
    EXPECT_CALL( getMockCloudStreamSaver(), createRecording ).Times( 0 );
    EXPECT_CALL( getMockCloudStreamSaver(), deleteRecording ).Times( 1 );
    EXPECT_CALL( getMockCloudStreamSaver(), isSavingCreated ).Times( 1 ).WillRepeatedly( Return( false ) );
    EXPECT_CALL( getMockLiveStreamer(), createCameraStream ).Times( 0 );
    EXPECT_CALL( getMockLiveStreamer(), deleteCameraStream ).Times( 0 );
    EXPECT_CALL( getMockStreamParticleRecorder(), deleteRecording ).WillRepeatedly( Return() );
    EXPECT_CALL( getMockStreamParticleRecorder(), isRecordingCreated ).WillRepeatedly( Return( false ) );
    EXPECT_CALL( getMockPostponedMethodManager(), addMethod( IPostponedMethodManager::MethodType::StartLiveRtsp, _ ) )
        .Times( 1 );
    EXPECT_CALL( getMockPostponedMethodManager(), removeMethod( IPostponedMethodManager::MethodType::StartLiveRtsp ) )
        .Times( 0 );
    EXPECT_CALL( getMockPostponedMethodManager(),
                 removeMethod( IPostponedMethodManager::MethodType::CreateSavingToCloud ) )
        .Times( 0 );
    EXPECT_CALL( getMockPostponedMethodManager(),
                 removeMethod( IPostponedMethodManager::MethodType::StartSavingToCloud ) )
        .Times( 0 );
    EXPECT_CALL( getMockPostponedMethodManager(), runAllMethods ).Times( 1 );
    auto& camera = getCamera();
    camera.createLiveRtspStream();
    std::this_thread::yield();
    clearConditionalVariable();
    camera.startStream();
    std::this_thread::yield();
    EXPECT_CALL( getMockCameraInput(), isRunning ).Times( 2 ).WillRepeatedly( Return( true ) );
    std::this_thread::yield();
    std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
    removeCamera();
}

TEST_F( CameraTestSuite, createPostponedCreateStreamSaving1 ) {
    EXPECT_CALL( getMockCameraInput(), start ).Times( 0 );
    EXPECT_CALL( getMockCameraInput(), isRunning ).Times( 2 ).WillRepeatedly( Return( false ) );
    EXPECT_CALL( getMockCloudStreamSaver(), createRecording ).Times( 0 );
    EXPECT_CALL( getMockCloudStreamSaver(), deleteRecording ).Times( 0 );
    EXPECT_CALL( getMockLiveStreamer(), deleteCameraStream ).Times( 0 );
    EXPECT_CALL( getMockCloudStreamSaver(), isSavingCreated ).WillOnce( Return( false ) );
    EXPECT_CALL( getMockStreamParticleRecorder(), deleteRecording ).WillRepeatedly( Return() );
    EXPECT_CALL( getMockPostponedMethodManager(),
                 addMethod( IPostponedMethodManager::MethodType::CreateSavingToCloud, _ ) )
        .Times( 1 );
    EXPECT_CALL( getMockPostponedMethodManager(), removeMethod( IPostponedMethodManager::MethodType::StartLiveRtsp ) )
        .Times( 0 );
    EXPECT_CALL( getMockPostponedMethodManager(),
                 removeMethod( IPostponedMethodManager::MethodType::CreateSavingToCloud ) )
        .Times( 1 );
    EXPECT_CALL( getMockPostponedMethodManager(),
                 removeMethod( IPostponedMethodManager::MethodType::StartSavingToCloud ) )
        .Times( 0 );
    EXPECT_CALL( getMockPostponedMethodManager(), runAllMethods ).Times( 0 );
    auto& camera = getCamera();
    camera.createStreamSavingToCloud(
        30, 5, 0, "", []( const FilePath&, const ResultCallback&, const FileRemoverSharedPtr&, const Json& ) {} );
    std::this_thread::yield();
    removeCamera();
}

TEST_F( CameraTestSuite, createPostponedCreateStreamSaving2 ) {
    EXPECT_CALL( getMockCameraInput(), start ).WillOnce( Return( true ) );
    EXPECT_CALL( getMockCameraInput(), isRunning ).WillRepeatedly( Return( false ) );
    EXPECT_CALL( getMockCloudStreamSaver(), createRecording ).Times( 0 );
    EXPECT_CALL( getMockCloudStreamSaver(), deleteRecording ).Times( 1 );
    EXPECT_CALL( getMockLiveStreamer(), deleteCameraStream ).Times( 0 );
    EXPECT_CALL( getMockCloudStreamSaver(), isSavingCreated ).Times( 2 ).WillRepeatedly( Return( false ) );
    EXPECT_CALL( getMockStreamParticleRecorder(), deleteRecording ).WillRepeatedly( Return() );
    EXPECT_CALL( getMockStreamParticleRecorder(), isRecordingCreated ).WillRepeatedly( Return( false ) );
    EXPECT_CALL( getMockPostponedMethodManager(),
                 addMethod( IPostponedMethodManager::MethodType::CreateSavingToCloud, _ ) )
        .Times( 1 );
    EXPECT_CALL( getMockPostponedMethodManager(), removeMethod( IPostponedMethodManager::MethodType::StartLiveRtsp ) )
        .Times( 0 );
    EXPECT_CALL( getMockPostponedMethodManager(),
                 removeMethod( IPostponedMethodManager::MethodType::CreateSavingToCloud ) )
        .Times( 0 );
    EXPECT_CALL( getMockPostponedMethodManager(),
                 removeMethod( IPostponedMethodManager::MethodType::StartSavingToCloud ) )
        .Times( 0 );
    EXPECT_CALL( getMockPostponedMethodManager(), runAllMethods ).Times( 1 );
    auto& camera = getCamera();
    camera.createStreamSavingToCloud(
        30, 5, 0, "", []( const FilePath&, const ResultCallback&, const FileRemoverSharedPtr&, const Json& ) {} );
    clearConditionalVariable();
    camera.startStream();
    std::this_thread::yield();
    EXPECT_CALL( getMockCameraInput(), isRunning ).Times( 2 ).WillRepeatedly( Return( true ) );
    std::this_thread::yield();
    std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
    removeCamera();
}

TEST_F( CameraTestSuite, createPostponedCreateStreamSaving3 ) {
    EXPECT_CALL( getMockCameraInput(), start ).WillOnce( Return( true ) );
    EXPECT_CALL( getMockCameraInput(), isRunning ).Times( 0 );
    EXPECT_CALL( getMockCloudStreamSaver(), createRecording ).Times( 1 );
    EXPECT_CALL( getMockCloudStreamSaver(), deleteRecording ).Times( 1 );
    EXPECT_CALL( getMockCloudStreamSaver(), isSavingCreated ).Times( 2 ).WillRepeatedly( Return( false ) );
    EXPECT_CALL( getMockLiveStreamer(), deleteCameraStream ).Times( 0 );
    EXPECT_CALL( getMockStreamParticleRecorder(), deleteRecording ).WillRepeatedly( Return() );
    EXPECT_CALL( getMockStreamParticleRecorder(), isRecordingCreated ).WillRepeatedly( Return( false ) );
    EXPECT_CALL( getMockPostponedMethodManager(),
                 addMethod( IPostponedMethodManager::MethodType::CreateSavingToCloud, _ ) )
        .Times( 0 );
    EXPECT_CALL( getMockPostponedMethodManager(), removeMethod( IPostponedMethodManager::MethodType::StartLiveRtsp ) )
        .Times( 0 );
    EXPECT_CALL( getMockPostponedMethodManager(),
                 removeMethod( IPostponedMethodManager::MethodType::CreateSavingToCloud ) )
        .Times( 0 );
    EXPECT_CALL( getMockPostponedMethodManager(),
                 removeMethod( IPostponedMethodManager::MethodType::StartSavingToCloud ) )
        .Times( 0 );
    EXPECT_CALL( getMockPostponedMethodManager(), runAllMethods ).Times( 1 );
    auto& camera = getCamera();
    camera.createStreamSavingToCloud(
        30, 0, 0, "", []( const FilePath&, const ResultCallback&, const FileRemoverSharedPtr&, const Json& ) {} );
    clearConditionalVariable();
    camera.startStream();
    std::this_thread::yield();
    EXPECT_CALL( getMockCameraInput(), isRunning ).Times( 2 ).WillRepeatedly( Return( true ) );
    std::this_thread::yield();
    std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
    removeCamera();
}

TEST_F( CameraTestSuite, createPostponedStartStreamSaving ) {
    EXPECT_CALL( getMockCameraInput(), start ).WillOnce( Return( true ) );
    EXPECT_CALL( getMockCameraInput(), isRunning ).WillRepeatedly( Return( false ) );
    EXPECT_CALL( getMockCloudStreamSaver(), isStarted ).Times( 2 ).WillRepeatedly( Return( false ) );
    EXPECT_CALL( getMockCloudStreamSaver(), deleteRecording ).Times( 1 );
    EXPECT_CALL( getMockLiveStreamer(), deleteCameraStream ).Times( 0 );
    EXPECT_CALL( getMockCloudStreamSaver(), isSavingCreated ).Times( 1 ).WillRepeatedly( Return( false ) );
    EXPECT_CALL( getMockStreamParticleRecorder(), deleteRecording ).WillRepeatedly( Return() );
    EXPECT_CALL( getMockStreamParticleRecorder(), isRecordingCreated ).WillRepeatedly( Return( false ) );
    EXPECT_CALL( getMockPostponedMethodManager(),
                 addMethod( IPostponedMethodManager::MethodType::CreateSavingToCloud, _ ) )
        .Times( 1 );
    EXPECT_CALL( getMockPostponedMethodManager(),
                 addMethod( IPostponedMethodManager::MethodType::StartSavingToCloud, _ ) )
        .Times( 1 );
    EXPECT_CALL( getMockPostponedMethodManager(), removeMethod( IPostponedMethodManager::MethodType::StartLiveRtsp ) )
        .Times( 0 );
    EXPECT_CALL( getMockPostponedMethodManager(),
                 removeMethod( IPostponedMethodManager::MethodType::CreateSavingToCloud ) )
        .Times( 0 );
    EXPECT_CALL( getMockPostponedMethodManager(),
                 removeMethod( IPostponedMethodManager::MethodType::StartSavingToCloud ) )
        .Times( 0 );
    EXPECT_CALL( getMockPostponedMethodManager(), runAllMethods ).Times( 1 );
    auto& camera = getCamera();
    camera.createStreamSavingToCloud(
        30, 5, 0, "", []( const FilePath&, const ResultCallback&, const FileRemoverSharedPtr&, const Json& ) {} );
    std::this_thread::yield();
    EXPECT_CALL( getMockCloudStreamSaver(), isSavingCreated ).Times( 2 ).WillRepeatedly( Return( true ) );
    camera.startStreamSavingToCloud(
        {}, []( const FilePath&, const ResultCallback&, const FileRemoverSharedPtr&, const Json& ) {} );
    clearConditionalVariable();
    camera.startStream();
    std::this_thread::yield();
    EXPECT_CALL( getMockCameraInput(), isRunning ).Times( 2 ).WillRepeatedly( Return( true ) );
    std::this_thread::yield();
    std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
    removeCamera();
}

TEST_F( CameraTestSuite, removePostponedLiveStream ) {
    EXPECT_CALL( getMockCameraInput(), start ).WillOnce( Return( true ) );
    EXPECT_CALL( getMockCameraInput(), isRunning ).WillRepeatedly( Return( false ) );
    EXPECT_CALL( getMockCloudStreamSaver(), deleteRecording ).Times( 1 );
    EXPECT_CALL( getMockCloudStreamSaver(), isSavingCreated ).Times( 1 ).WillRepeatedly( Return( false ) );
    EXPECT_CALL( getMockLiveStreamer(), createCameraStream ).Times( 0 );
    EXPECT_CALL( getMockLiveStreamer(), deleteCameraStream ).Times( 0 );
    EXPECT_CALL( getMockStreamParticleRecorder(), deleteRecording ).WillRepeatedly( Return() );
    EXPECT_CALL( getMockStreamParticleRecorder(), isRecordingCreated ).WillRepeatedly( Return( false ) );
    EXPECT_CALL( getMockPostponedMethodManager(), addMethod( IPostponedMethodManager::MethodType::StartLiveRtsp, _ ) )
        .Times( 1 );
    EXPECT_CALL( getMockPostponedMethodManager(), removeMethod( IPostponedMethodManager::MethodType::StartLiveRtsp ) )
        .Times( 1 );
    EXPECT_CALL( getMockPostponedMethodManager(),
                 removeMethod( IPostponedMethodManager::MethodType::CreateSavingToCloud ) )
        .Times( 0 );
    EXPECT_CALL( getMockPostponedMethodManager(),
                 removeMethod( IPostponedMethodManager::MethodType::StartSavingToCloud ) )
        .Times( 0 );
    EXPECT_CALL( getMockPostponedMethodManager(), runAllMethods ).Times( 1 );
    auto& camera = getCamera();
    camera.createLiveRtspStream();
    camera.deleteLiveRtspStream();
    clearConditionalVariable();
    camera.startStream();
    std::this_thread::yield();
    EXPECT_CALL( getMockCameraInput(), isRunning ).Times( 2 ).WillRepeatedly( Return( true ) );
    std::this_thread::yield();
    std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
    removeCamera();
}

TEST_F( CameraTestSuite, removePostponedCreateStreamSaving ) {
    EXPECT_CALL( getMockCameraInput(), start ).WillOnce( Return( true ) );
    EXPECT_CALL( getMockCameraInput(), isRunning ).WillRepeatedly( Return( false ) );
    EXPECT_CALL( getMockCloudStreamSaver(), deleteRecording ).Times( 1 );
    EXPECT_CALL( getMockLiveStreamer(), deleteCameraStream ).Times( 0 );
    EXPECT_CALL( getMockCloudStreamSaver(), isSavingCreated ).Times( 2 ).WillRepeatedly( Return( false ) );
    EXPECT_CALL( getMockStreamParticleRecorder(), deleteRecording ).WillRepeatedly( Return() );
    EXPECT_CALL( getMockStreamParticleRecorder(), isRecordingCreated ).WillRepeatedly( Return( false ) );
    EXPECT_CALL( getMockPostponedMethodManager(),
                 addMethod( IPostponedMethodManager::MethodType::CreateSavingToCloud, _ ) )
        .Times( 1 );
    EXPECT_CALL( getMockPostponedMethodManager(), removeMethod( IPostponedMethodManager::MethodType::StartLiveRtsp ) )
        .Times( 0 );
    EXPECT_CALL( getMockPostponedMethodManager(),
                 removeMethod( IPostponedMethodManager::MethodType::CreateSavingToCloud ) )
        .Times( 1 );
    EXPECT_CALL( getMockPostponedMethodManager(),
                 removeMethod( IPostponedMethodManager::MethodType::StartSavingToCloud ) )
        .Times( 0 );
    EXPECT_CALL( getMockPostponedMethodManager(), runAllMethods ).Times( 1 );
    auto& camera = getCamera();
    camera.createStreamSavingToCloud(
        30, 5, 0, "", []( const FilePath&, const ResultCallback&, const FileRemoverSharedPtr&, const Json& ) {} );
    camera.deleteStreamSavingToCloud();
    clearConditionalVariable();
    camera.startStream();
    std::this_thread::yield();
    EXPECT_CALL( getMockCameraInput(), isRunning ).Times( 2 ).WillRepeatedly( Return( true ) );
    std::this_thread::yield();
    std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
    removeCamera();
}

TEST_F( CameraTestSuite, removePostponedStartStreamSaving ) {
    EXPECT_CALL( getMockCameraInput(), start ).WillOnce( Return( true ) );
    EXPECT_CALL( getMockCameraInput(), isRunning ).WillRepeatedly( Return( false ) );
    EXPECT_CALL( getMockCloudStreamSaver(), deleteRecording ).Times( 1 );
    EXPECT_CALL( getMockLiveStreamer(), deleteCameraStream ).Times( 0 );
    EXPECT_CALL( getMockCloudStreamSaver(), isSavingCreated ).Times( 1 ).WillRepeatedly( Return( false ) );
    EXPECT_CALL( getMockCloudStreamSaver(), isStarted ).Times( 1 );
    EXPECT_CALL( getMockStreamParticleRecorder(), deleteRecording ).WillRepeatedly( Return() );
    EXPECT_CALL( getMockStreamParticleRecorder(), isRecordingCreated ).WillRepeatedly( Return( false ) );
    EXPECT_CALL( getMockPostponedMethodManager(),
                 addMethod( IPostponedMethodManager::MethodType::CreateSavingToCloud, _ ) )
        .Times( 1 );
    EXPECT_CALL( getMockPostponedMethodManager(),
                 addMethod( IPostponedMethodManager::MethodType::StartSavingToCloud, _ ) )
        .Times( 1 );
    EXPECT_CALL( getMockPostponedMethodManager(), removeMethod( IPostponedMethodManager::MethodType::StartLiveRtsp ) )
        .Times( 0 );
    EXPECT_CALL( getMockPostponedMethodManager(),
                 removeMethod( IPostponedMethodManager::MethodType::CreateSavingToCloud ) )
        .Times( 1 );
    EXPECT_CALL( getMockPostponedMethodManager(),
                 removeMethod( IPostponedMethodManager::MethodType::StartSavingToCloud ) )
        .Times( 1 );
    EXPECT_CALL( getMockPostponedMethodManager(), runAllMethods ).Times( 1 );
    auto& camera = getCamera();
    camera.createStreamSavingToCloud(
        30, 5, 0, "", []( const FilePath&, const ResultCallback&, const FileRemoverSharedPtr&, const Json& ) {} );
    EXPECT_CALL( getMockCloudStreamSaver(), isSavingCreated ).Times( 2 ).WillRepeatedly( Return( true ) );
    camera.startStreamSavingToCloud(
        {}, []( const FilePath&, const ResultCallback&, const FileRemoverSharedPtr&, const Json& ) {} );
    camera.stopStreamSavingToCloud();
    camera.deleteStreamSavingToCloud();
    clearConditionalVariable();
    camera.startStream();
    std::this_thread::yield();
    EXPECT_CALL( getMockCloudStreamSaver(), isSavingCreated ).Times( 1 ).WillRepeatedly( Return( false ) );
    EXPECT_CALL( getMockCameraInput(), isRunning ).Times( 2 ).WillRepeatedly( Return( true ) );
    std::this_thread::yield();
    std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
    removeCamera();
}

TEST_F( CameraTestSuite, makeSnapshotForUI1 ) {
    EXPECT_CALL( getMockCameraInput(), isRunning ).Times( 2 ).WillRepeatedly( Return( false ) );
    EXPECT_CALL( getMockStreamParticleRecorder(), deleteRecording ).WillRepeatedly( Return() );
    EXPECT_CALL( getMockPostponedMethodManager(), removeMethod( IPostponedMethodManager::MethodType::StartLiveRtsp ) )
        .Times( 0 );
    EXPECT_CALL( getMockPostponedMethodManager(),
                 removeMethod( IPostponedMethodManager::MethodType::CreateSavingToCloud ) )
        .Times( 1 );
    EXPECT_CALL( getMockSnapshotManager(), makeSnapshotForUI ).Times( 0 );
    auto& camera = getCamera();
    Json responseMessage;
    camera.makeSnapshotForUI( nullptr, "", [&responseMessage]( const Json& response ) { responseMessage = response; } );
    std::this_thread::yield();
    EXPECT_EQ( responseMessage,
               JSON( "error" << JSON( "message"
                                      << "Stream is not started" ) ) );
}

TEST_F( CameraTestSuite, makeSnapshotForUI2 ) {
    EXPECT_CALL( getMockCameraInput(), isRunning ).Times( 2 ).WillRepeatedly( Return( true ) );
    EXPECT_CALL( getMockSnapshotManager(), makeSnapshotForUI ).Times( 1 );
    EXPECT_CALL( getMockCloudStreamSaver(), deleteRecording ).Times( 1 );
    EXPECT_CALL( getMockStreamParticleRecorder(), deleteRecording ).WillRepeatedly( Return() );
    auto& camera = getCamera();
    camera.makeSnapshotForUI( nullptr, "", []( const Json& ) {} );
    std::this_thread::yield();
}

TEST_F( CameraTestSuite, doNotStartLiveRTSTPStreamDuringCameraStreamStarting ) {
    EXPECT_CALL( getMockCameraInput(), start ).WillOnce( Return( true ) );
    EXPECT_CALL( getMockCloudStreamSaver(), isSavingCreated ).Times( 1 ).WillRepeatedly( Return( false ) );
    EXPECT_CALL( getMockCloudStreamSaver(), deleteRecording ).WillRepeatedly( Return() );
    EXPECT_CALL( getMockStreamParticleRecorder(), deleteRecording ).WillRepeatedly( Return() );
    EXPECT_CALL( getMockStreamParticleRecorder(), isRecordingCreated ).WillRepeatedly( Return( false ) );
    EXPECT_CALL( getMockPostponedMethodManager(), runAllMethods ).WillRepeatedly( Return() );

    EXPECT_CALL( getMockLiveRTSPStreamStatusNotifier(), onRTSPStreamStarted ).Times( 0 );

    auto& camera = getCamera();
    clearConditionalVariable();
    camera.startStream();
    std::this_thread::yield();

    EXPECT_CALL( getMockCameraInput(), isRunning ).Times( 2 ).WillRepeatedly( Return( true ) );
    std::this_thread::yield();
    std::this_thread::sleep_for( std::chrono::milliseconds( 300 ) );
    removeCamera();
}

TEST_F( CameraTestSuite, startLiveRTSTPStreamIfItSPaused ) {
    EXPECT_CALL( getMockCameraInput(), start ).WillOnce( Return( true ) );
    EXPECT_CALL( getMockCloudStreamSaver(), isSavingCreated ).Times( 1 ).WillRepeatedly( Return( false ) );
    EXPECT_CALL( getMockCloudStreamSaver(), deleteRecording ).WillRepeatedly( Return() );
    EXPECT_CALL( getMockStreamParticleRecorder(), deleteRecording ).WillRepeatedly( Return() );
    EXPECT_CALL( getMockStreamParticleRecorder(), isRecordingCreated ).WillRepeatedly( Return( false ) );
    EXPECT_CALL( getMockPostponedMethodManager(), runAllMethods ).WillRepeatedly( Return() );

    EXPECT_CALL( getMockLiveRTSPStreamStatusNotifier(), onRTSPStreamStarted ).Times( 1 );

    getLiveRTSPStreamStatus() = LiveRTSPStreamStatus::PAUSED;
    auto& camera = getCamera();
    clearConditionalVariable();
    camera.startStream();
    std::this_thread::yield();

    EXPECT_CALL( getMockCameraInput(), isRunning ).Times( 2 ).WillRepeatedly( Return( true ) );
    std::this_thread::yield();
    std::this_thread::sleep_for( std::chrono::milliseconds( 300 ) );
    removeCamera();
}

TEST_F( CameraTestSuite, doNotStartLiveRTSTPStreamIfItSPaused ) {
    EXPECT_CALL( getMockCameraInput(), isRunning ).Times( 1 ).WillRepeatedly( Return( true ) );
    EXPECT_CALL( getMockLiveStreamer(), createCameraStream ).Times( 0 );
    EXPECT_CALL( getMockCloudStreamSaver(), deleteRecording ).WillRepeatedly( Return() );
    EXPECT_CALL( getMockLiveRTSPStreamStatusNotifier(), onRTSPStreamStarted ).Times( 0 );
    EXPECT_CALL( getMockStreamParticleRecorder(), deleteRecording ).WillRepeatedly( Return() );

    getLiveRTSPStreamStatus() = LiveRTSPStreamStatus::PAUSED;

    auto& camera = getCamera();
    camera.createLiveRtspStream();
    std::this_thread::yield();
    removeCamera();
}

TEST_F( CameraTestSuite, doNotStartLiveRTSTPStreamIfItSRunning ) {
    EXPECT_CALL( getMockCameraInput(), isRunning ).Times( 1 ).WillRepeatedly( Return( true ) );
    EXPECT_CALL( getMockCloudStreamSaver(), deleteRecording ).WillRepeatedly( Return() );
    EXPECT_CALL( getMockLiveStreamer(), createCameraStream ).Times( 0 );
    EXPECT_CALL( getMockLiveRTSPStreamStatusNotifier(), onRTSPStreamStarted ).Times( 0 );
    EXPECT_CALL( getMockStreamParticleRecorder(), deleteRecording ).WillRepeatedly( Return() );

    getLiveRTSPStreamStatus() = LiveRTSPStreamStatus::RUNNING;

    auto& camera = getCamera();
    camera.createLiveRtspStream();
    std::this_thread::yield();
    removeCamera();
}

TEST_F( CameraTestSuite, startLiveRTSTPStreamIfItSStopped ) {
    EXPECT_CALL( getMockCameraInput(), isRunning ).Times( 2 ).WillRepeatedly( Return( true ) );
    EXPECT_CALL( getMockCloudStreamSaver(), deleteRecording ).WillRepeatedly( Return() );
    EXPECT_CALL( getMockLiveStreamer(), createCameraStream ).Times( 1 );
    EXPECT_CALL( getMockLiveRTSPStreamStatusNotifier(), onRTSPStreamStarted ).Times( 1 );
    EXPECT_CALL( getMockStreamParticleRecorder(), deleteRecording ).WillRepeatedly( Return() );

    getLiveRTSPStreamStatus() = LiveRTSPStreamStatus::STOPPED;

    auto& camera = getCamera();
    camera.createLiveRtspStream();
    std::this_thread::yield();
    removeCamera();
}

TEST_F( CameraTestSuite, doNotStopLiveRTSTPStreamIfItSStopped ) {
    EXPECT_CALL( getMockCameraInput(), isRunning ).Times( 2 ).WillRepeatedly( Return( true ) );
    EXPECT_CALL( getMockCloudStreamSaver(), deleteRecording ).WillRepeatedly( Return() );
    EXPECT_CALL( getMockLiveStreamer(), deleteCameraStream ).Times( 0 );
    EXPECT_CALL( getMockLiveRTSPStreamStatusNotifier(), onRTSPStreamStopped ).Times( 0 );
    EXPECT_CALL( getMockStreamParticleRecorder(), deleteRecording ).WillRepeatedly( Return() );

    getLiveRTSPStreamStatus() = LiveRTSPStreamStatus::STOPPED;

    auto& camera = getCamera();
    camera.deleteLiveRtspStream();
    std::this_thread::yield();
    removeCamera();
}

TEST_F( CameraTestSuite, stopLiveRTSTPStreamIfItSRunning ) {
    EXPECT_CALL( getMockCameraInput(), isRunning ).Times( 2 ).WillRepeatedly( Return( true ) );
    EXPECT_CALL( getMockCloudStreamSaver(), deleteRecording ).WillRepeatedly( Return() );
    EXPECT_CALL( getMockLiveStreamer(), deleteCameraStream ).Times( 1 );
    EXPECT_CALL( getMockLiveRTSPStreamStatusNotifier(), onRTSPStreamStopped ).Times( 1 );
    EXPECT_CALL( getMockStreamParticleRecorder(), deleteRecording ).WillRepeatedly( Return() );

    getLiveRTSPStreamStatus() = LiveRTSPStreamStatus::RUNNING;

    auto& camera = getCamera();
    camera.deleteLiveRtspStream();
    std::this_thread::yield();
    removeCamera();
}

TEST_F( CameraTestSuite, stopLiveRTSTPStreamIfItSPaused ) {
    EXPECT_CALL( getMockCameraInput(), isRunning ).Times( 2 ).WillRepeatedly( Return( true ) );
    EXPECT_CALL( getMockCloudStreamSaver(), deleteRecording ).WillRepeatedly( Return() );
    EXPECT_CALL( getMockLiveStreamer(), deleteCameraStream ).Times( 1 );
    EXPECT_CALL( getMockLiveRTSPStreamStatusNotifier(), onRTSPStreamStopped ).Times( 1 );
    EXPECT_CALL( getMockStreamParticleRecorder(), deleteRecording ).WillRepeatedly( Return() );

    getLiveRTSPStreamStatus() = LiveRTSPStreamStatus::PAUSED;

    auto& camera = getCamera();
    camera.deleteLiveRtspStream();
    std::this_thread::yield();
    removeCamera();
}

TEST_F( CameraTestSuite, liveStreamAutomaticallyDeletedIfInputWasStopped ) {
    EXPECT_CALL( getMockCameraInput(), isRunning ).WillOnce( Return( true ) );
    EXPECT_CALL( getMockCloudStreamSaver(), deleteRecording ).WillRepeatedly( Return() );
    EXPECT_CALL( getMockCloudStreamSaver(), isSavingCreated ).WillRepeatedly( Return( false ) );
    EXPECT_CALL( getMockLiveStreamer(), createCameraStream ).Times( 1 );
    EXPECT_CALL( getMockLiveStreamer(), deleteCameraStream ).Times( 0 );
    EXPECT_CALL( getMockStreamParticleRecorder(), deleteRecording ).WillRepeatedly( Return() );
    EXPECT_CALL( getMockStreamParticleRecorder(), isMakingRecord ).WillOnce( Return( false ) );
    EXPECT_CALL( getMockLiveRTSPStreamStatusNotifier(), onRTSPStreamStarted ).WillRepeatedly( Return() );
    EXPECT_CALL( getMockLiveRTSPStreamStatusNotifier(), onRTSPStreamStopped ).WillRepeatedly( Return() );
    EXPECT_CALL( getMockPostponedMethodManager(), addMethod( IPostponedMethodManager::MethodType::StartLiveRtsp, _ ) )
        .Times( 0 );
    EXPECT_CALL( getMockPostponedMethodManager(), removeMethod( IPostponedMethodManager::MethodType::StartLiveRtsp ) )
        .Times( 0 );
    EXPECT_CALL( getMockPostponedMethodManager(),
                 removeMethod( IPostponedMethodManager::MethodType::CreateSavingToCloud ) )
        .Times( 1 );

    auto& camera = getCamera();
    camera.createLiveRtspStream();
    std::this_thread::yield();
    EXPECT_EQ( getLiveRTSPStreamStatus(), LiveRTSPStreamStatus::RUNNING );

    EXPECT_CALL( getMockCameraInput(), isRunning ).Times( 1 ).WillRepeatedly( Return( false ) );
    triggerStreamReadingStopCallback();

    std::this_thread::yield();
    removeCamera();
}

TEST_F( CameraTestSuite, liveStreamDeletedOnRequestIfInputWasStopped ) {
    EXPECT_CALL( getMockCameraInput(), isRunning ).WillOnce( Return( true ) );
    EXPECT_CALL( getMockCloudStreamSaver(), isSavingCreated ).WillRepeatedly( Return( false ) );
    EXPECT_CALL( getMockLiveStreamer(), createCameraStream ).Times( 1 );
    EXPECT_CALL( getMockLiveStreamer(), deleteCameraStream ).Times( 1 );
    EXPECT_CALL( getMockStreamParticleRecorder(), deleteRecording ).WillRepeatedly( Return() );
    EXPECT_CALL( getMockStreamParticleRecorder(), isMakingRecord ).WillOnce( Return( false ) );
    EXPECT_CALL( getMockLiveRTSPStreamStatusNotifier(), onRTSPStreamStarted ).WillRepeatedly( Return() );
    EXPECT_CALL( getMockLiveRTSPStreamStatusNotifier(), onRTSPStreamStopped ).WillRepeatedly( Return() );
    EXPECT_CALL( getMockPostponedMethodManager(), addMethod( IPostponedMethodManager::MethodType::StartLiveRtsp, _ ) )
        .Times( 0 );
    EXPECT_CALL( getMockPostponedMethodManager(), removeMethod( IPostponedMethodManager::MethodType::StartLiveRtsp ) )
        .Times( 1 );
    EXPECT_CALL( getMockPostponedMethodManager(),
                 removeMethod( IPostponedMethodManager::MethodType::CreateSavingToCloud ) )
        .Times( 1 );

    auto& camera = getCamera();
    camera.createLiveRtspStream();
    std::this_thread::yield();
    EXPECT_EQ( getLiveRTSPStreamStatus(), LiveRTSPStreamStatus::RUNNING );

    EXPECT_CALL( getMockCameraInput(), isRunning ).Times( 2 ).WillRepeatedly( Return( false ) );
    triggerStreamReadingStopCallback();
    camera.deleteLiveRtspStream();

    std::this_thread::yield();
    removeCamera();
}

}  // namespace UT
}  // namespace HubPlatform
