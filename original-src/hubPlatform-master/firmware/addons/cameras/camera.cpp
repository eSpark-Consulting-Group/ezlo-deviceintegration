#include "cameras/camera.h"
#include "cameras/storages/local_storage.h"
#include "cameras/live_rtsp_stream_status_notifier_interface.h"

#include "cameras/postponed/postponed_method_manager_interface.h"
#include "cameras/local_stream/live_streamer_interface.h"
#include "cameras/cloud/cloud_stream_saver_interface.h"
#include "cameras/cloud/stream_particle_recorder_interface.h"
#include "cameras/snapshot/snapshot_manager_interface.h"

#include "base_utils/rest_client.h"
#include "base_utils/utils.h"
#include "common_utils/cpp14.h"
#include "log_utils/ezlo_log.h"
#include "jsonrpc/jsonrpc.h"

#include <string>
#include <utility>

namespace HubPlatform {

constexpr std::chrono::seconds Camera::DefaultStartStreamTriesSleepTime;

Camera::Camera( std::string streamId,
                std::string ipAddress,
                std::unique_ptr< ICameraInput > cameraInput,
                std::unique_ptr< IPostponedMethodManager > postponedMethodManager,
                std::unique_ptr< ILiveStreamer > liveStreamer,
                std::unique_ptr< ICloudStreamSaver > cloudStreamSaver,
                std::unique_ptr< IStreamParticleRecorder > particleRecorder,
                std::unique_ptr< ISnapshotManager > snapshotManager,
                ILiveRTSPStreamStatusNotifier& liveRTSPStreamStatusNotifier,
                RtspTransports tryConnectWithProtocols )
: m_streamId( streamId )
, m_postponedMethodManager( std::move( postponedMethodManager ) )
, m_liveStreamer( std::move( liveStreamer ) )
, m_cloudStreamSaver( std::move( cloudStreamSaver ) )
, m_particleRecorder( std::move( particleRecorder ) )
, m_snapshotManager( std::move( snapshotManager ) )
, m_liveRtspStreamLink( "rtsp://" + std::move( ipAddress ) + "/" + streamId )
, m_cameraInput( std::move( cameraInput ) )
, m_liveRTSPStreamStatusNotifier( liveRTSPStreamStatusNotifier )
, m_rtspTransports( std::move( tryConnectWithProtocols ) ) {
    m_cameraInput->setEventsListener( this );
}

Camera::~Camera() {
    LOGGER_DEBUG( cc::debug( "Releasing camera object for stream: " ) + cc::note( m_streamId ) );
    try {
        deleteStreamSavingToCloud();
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Delete cloud stream recording exception: " ) + cc::attention( ex.what() ) );
    }
    try {
        deleteStreamParticleRecording();
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Delete stream preview recording exception: " ) + cc::attention( ex.what() ) );
    }
    LOGGER_DEBUG( cc::debug( "Stopping stream thread" ) );
    m_isStartStreamCanRun = false;
    m_streamRestartingCondVar.notify_all();
    m_surpriseTriggeringBlocker = true;
    m_startingCondVar.notify_all();
    if( m_startStreamThread.joinable() ) {
        LOGGER_DEBUG( cc::debug( "Joining stream thread" ) );
        m_startStreamThread.join();
    }
    LOGGER_DEBUG( cc::debug( "Stopping camera input for stream: " ) + cc::note( m_streamId ) );
    m_cameraInput->stop();
    m_cameraInput.reset();

    LOGGER_DEBUG( cc::debug( "Releasing camera object components for stream: " ) + cc::note( m_streamId ) );
}

void Camera::startStream() {
    if( !m_isStartStreamCanRun || m_startStreamThread.joinable() ) {
        return;
    }
    m_startStreamThread = std::thread( [this]() {
        LOGGER_INFO( cc::info( "Camera stream thread is started: streamId: " ) + cc::note( m_streamId ) +
                     cc::info( ", thread id: " ) + cc::threadId() );
        while( m_isStartStreamCanRun ) {
            while( m_isStartStreamCanRun && !startStreamWithDifferentTransport() ) {
                std::unique_lock< std::mutex > lock( m_streamRestartingMutex );
                auto spuriousWakeUpPredicate = [this]() { return !m_isStartStreamCanRun; };
                m_streamRestartingCondVar.wait_for( lock, m_startStreamTriesSleepTime, spuriousWakeUpPredicate );
            }
            std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
            if( !m_cameraInput->isRunning() ) {
                continue;
            }

            if( !m_isStartStreamCanRun ) {
                return;
            }

            Log::info( cc::info( "Stream is started: " ) + cc::attention( m_streamId ) );

            {
                std::lock_guard< std::recursive_mutex > guard( m_methodMutex );
                if( m_cloudStreamSaver->isSavingCreated() ) {
                    if( m_cloudStreamSaver->isStarted() ) {
                        m_cloudStreamSaver->startRecording();
                    }
                }
                if( m_particleRecorder->isRecordingCreated() && m_particleRecordingWasForceStopped ) {
                    m_particleRecorder->makeRecord();
                    m_particleRecordingWasForceStopped = false;
                }
                m_postponedMethodManager->runAllMethods();

                if( m_liveRTSPStreamStatus == LiveRTSPStreamStatus::PAUSED ) {
                    m_liveRTSPStreamStatus = LiveRTSPStreamStatus::RUNNING;
                    m_liveRTSPStreamStatusNotifier.onRTSPStreamStarted( m_streamId, m_liveRtspStreamLink );
                }
            }

            std::unique_lock< std::mutex > lock( m_startingMutex );
            m_startingCondVar.wait( lock, [this]() { return m_surpriseTriggeringBlocker.load(); } );
            m_surpriseTriggeringBlocker = false;
        }
    } );
    waitForCondition(
        [this] {
            std::this_thread::yield();
            return m_startStreamThread.joinable();
        },
        std::chrono::seconds{ 5 },
        std::chrono::milliseconds{ 50 } );
}

void Camera::createLiveRtspStream() {
    std::lock_guard< std::recursive_mutex > guard( m_methodMutex );

    if( m_liveRTSPStreamStatus == LiveRTSPStreamStatus::RUNNING ||
        m_liveRTSPStreamStatus == LiveRTSPStreamStatus::PAUSED ) {
        return;
    }

    if( !m_cameraInput->isRunning() ) {
        Log::debug( cc::debug( "Postpone live stream creating: " ) + cc::attention( m_streamId ) );
        m_postponedMethodManager->addMethod( IPostponedMethodManager::MethodType::StartLiveRtsp,
                                             [this]() { createLiveRtspStream(); } );
        m_surpriseTriggeringBlocker = true;
        m_startingCondVar.notify_all();
        return;
    }
    try {
        m_liveStreamer->createCameraStream();

        m_liveRTSPStreamStatus = LiveRTSPStreamStatus::RUNNING;
        m_liveRTSPStreamStatusNotifier.onRTSPStreamStarted( m_streamId, m_liveRtspStreamLink );
    }
    catch( const std::exception& ex ) {
        m_liveRTSPStreamStatusNotifier.onRTSPStreamCreationFailed( m_streamId, ex.what() );
    }
}

void Camera::deleteLiveRtspStream() {
    std::lock_guard< std::recursive_mutex > guard( m_methodMutex );

    if( !m_cameraInput->isRunning() ) {
        Log::debug( cc::debug( "Delete postponed live stream: " ) + cc::attention( m_streamId ) );
        m_postponedMethodManager->removeMethod( IPostponedMethodManager::MethodType::StartLiveRtsp );
    }
    if( m_liveRTSPStreamStatus == LiveRTSPStreamStatus::STOPPED ) {
        return;
    }

    m_liveStreamer->deleteCameraStream();

    m_liveRTSPStreamStatus = LiveRTSPStreamStatus::STOPPED;
    m_liveRTSPStreamStatusNotifier.onRTSPStreamStopped( m_streamId );
}

void Camera::createStreamSavingToCloud( int segmentDurationSec,
                                        int64_t secondsBeforeStart,
                                        int segmentsLimit,
                                        const std::string& format,
                                        const SaveFileCallback& saveFile ) {
    std::lock_guard< std::recursive_mutex > guard( m_methodMutex );
    if( m_cloudStreamSaver->isSavingCreated() ) {
        Log::warn( cc::warn( "Stream Saving To Cloud already exists" ) );
        return;
    }
    if( secondsBeforeStart > 0 && !m_cameraInput->isRunning() ) {
        Log::debug( cc::debug( "Postpone stream saving to cloud creating: " ) + cc::attention( m_streamId ) );
        m_postponedMethodManager->addMethod(
            IPostponedMethodManager::MethodType::CreateSavingToCloud,
            [this, segmentDurationSec, secondsBeforeStart, segmentsLimit, format, saveFile]() {
                createStreamSavingToCloud( segmentDurationSec, secondsBeforeStart, segmentsLimit, format, saveFile );
            } );
        m_surpriseTriggeringBlocker = true;
        m_startingCondVar.notify_all();
        return;
    }
    m_cloudStreamSaver->createRecording( segmentDurationSec, secondsBeforeStart, segmentsLimit, format, saveFile );
}

void Camera::deleteStreamSavingToCloud() {
    std::lock_guard< std::recursive_mutex > guard( m_methodMutex );
    LOGGER_DEBUG_RAW( cc::debug( "Camera: Deleting stream saving to cloud for stream: " ) + cc::info( m_streamId ) );
    if( !m_cameraInput->isRunning() ) {
        Log::debug( cc::debug( "Delete postponed stream saving to cloud creating: " ) + cc::attention( m_streamId ) );
        m_postponedMethodManager->removeMethod( IPostponedMethodManager::MethodType::CreateSavingToCloud );
        return;
    }
    LOGGER_DEBUG_RAW( cc::debug( "Camera: Deleting stream recording: " ) + cc::info( m_streamId ) );
    m_cloudStreamSaver->deleteRecording();
}

void Camera::startStreamSavingToCloud( const Json& metaData, const SaveFileCallback& saveFile ) {
    std::lock_guard< std::recursive_mutex > guard( m_methodMutex );
    if( !m_cloudStreamSaver->isSavingCreated() ) {
        Log::warn( cc::warn( "Stream Saving To Cloud was not created" ) );
        return;
    }
    if( m_cloudStreamSaver->isStarted() ) {
        Log::warn( cc::warn( "Stream Saving To Cloud was already started" ) );
        return;
    }
    if( !m_cameraInput->isRunning() ) {
        Log::debug( cc::debug( "Postpone stream saving to cloud starting: " ) + cc::attention( m_streamId ) );
        m_postponedMethodManager->addMethod(
            IPostponedMethodManager::MethodType::StartSavingToCloud,
            [this, metaData, saveFile]() { startStreamSavingToCloud( metaData, saveFile ); } );
        m_surpriseTriggeringBlocker = true;
        m_startingCondVar.notify_all();
        return;
    }
    m_cloudStreamSaver->startRecording( metaData, saveFile );
}

void Camera::stopStreamSavingToCloud() {
    std::lock_guard< std::recursive_mutex > guard( m_methodMutex );
    if( !m_cloudStreamSaver->isSavingCreated() ) {
        Log::warn( cc::warn( "Stream Saving To Cloud was not created" ) );
        return;
    }
    if( !m_cameraInput->isRunning() ) {
        Log::debug( cc::debug( "Delete postponed stream saving to cloud starting: " ) + cc::attention( m_streamId ) );
        m_postponedMethodManager->removeMethod( IPostponedMethodManager::MethodType::StartSavingToCloud );
        return;
    }
    m_cloudStreamSaver->stopRecording();
}

void Camera::createStreamParticleRecording( const StreamParticleSettings& settings, const SaveFileCallback& saveFile ) {
    std::lock_guard< std::recursive_mutex > guard( m_methodMutex );
    if( m_particleRecorder->isRecordingCreated() ) {
        Log::warn( cc::warn( "Stream preview recording already created" ) );
        return;
    }
    m_particleRecorder->createRecording( settings, saveFile );
}

void Camera::deleteStreamParticleRecording() {
    std::lock_guard< std::recursive_mutex > guard( m_methodMutex );
    LOGGER_DEBUG_RAW( cc::debug( "Camera: Deleting stream particle recording: " ) + cc::info( m_streamId ) );
    m_particleRecorder->deleteRecording();
}

void Camera::recordStreamParticle( const Json& metaData ) {
    std::lock_guard< std::recursive_mutex > guard( m_methodMutex );
    if( !m_particleRecorder->isRecordingCreated() ) {
        Log::warn( cc::warn( "Stream preview recording was not created" ) );
        return;
    }
    if( m_particleRecorder->isMakingRecord() ) {
        Log::warn( cc::warn( "Recording of preview video is already running" ) );
        return;
    }
    if( !m_cameraInput->isRunning() ) {
        Log::debug( cc::debug( "Postpone recording of stream particle: " ) + cc::attention( m_streamId ) );
        auto callback = [this, metaData]() { recordStreamParticle( metaData ); };
        m_postponedMethodManager->addMethod( IPostponedMethodManager::MethodType::StartParticleRecording, callback );
        m_surpriseTriggeringBlocker = true;
        m_startingCondVar.notify_all();
        return;
    }
    m_particleRecorder->makeRecord( metaData );
    m_particleRecordingWasForceStopped = false;
}

void Camera::stopStreamParticleRecording() {
    std::lock_guard< std::recursive_mutex > guard( m_methodMutex );
    if( !m_particleRecorder->isRecordingCreated() ) {
        Log::warn( cc::warn( "Stream particle recording was not created" ) );
        return;
    }
    if( !m_cameraInput->isRunning() ) {
        Log::debug( cc::debug( "Delete postponed stream particle recording start: " ) + cc::attention( m_streamId ) );
        m_postponedMethodManager->removeMethod( IPostponedMethodManager::MethodType::StartParticleRecording );
        return;
    }
    m_particleRecorder->stopMakingRecord();
}

bool Camera::isRecordingStreamParticle() const noexcept {
    return m_particleRecorder->isMakingRecord();
}

bool Camera::isRunning() const {
    return m_cameraInput->isRunning();
}

void Camera::makeSnapshotForCloud( const Json& metaData,
                                   SaveFileCallback saveFile,
                                   const std::string& snapshotId,
                                   const std::function< void( const Json& ) >& onFinished,
                                   const std::function< void( const std::string& ) >& onError ) {
    if( !m_cameraInput->isRunning() ) {
        const std::string& message = "Stream is not started";
        Log::warn( cc::warn( message ) );
        onError( message );
        return;
    }
    m_snapshotManager->makeSnapshotForCloud(
        metaData, std::move( saveFile ), snapshotId, [onFinished, onError]( const Json& message ) {
            if( hasError( message ) ) {
                onError( message[ "error" ] );
            }
            else {
                onFinished( message );
            }
        } );
}

void Camera::makeSnapshotForUI( std::shared_ptr< Resolution > resolution,
                                const std::string& requestId,
                                const std::function< void( const Json& ) >& response ) {
    if( !m_cameraInput->isRunning() ) {
        const std::string& message = "Stream is not started";
        Log::warn( cc::warn( message ) );
        response( JSON( "error" << JSON( "message" << message ) ) );
        return;
    }
    m_snapshotManager->makeSnapshotForUI( std::move( resolution ), requestId, response );
}

std::string Camera::getLiveRtspStreamLink() const {
    return m_liveRtspStreamLink;
}

void Camera::setLiveRtspStreamLink( const std::string& link ) {
    std::lock_guard< std::recursive_mutex > guard( m_methodMutex );
    m_liveRtspStreamLink = link;
}

std::string Camera::getStreamId() const {
    return m_streamId;
}

void Camera::onInputReadingStopped( int errorCode ) {
    onStreamReadingStop();
    m_liveRTSPStreamStatusNotifier.onRTSPStreamError( m_streamId, StreamError::CAMERA_STREAM_CLOSED );
}

void Camera::onAccumulatedDelayThresholdReached() {
    onStreamReadingStop();
    m_liveRTSPStreamStatusNotifier.onRTSPStreamError( m_streamId, StreamError::CAMERA_ACCUMULATED_DELAY );
}

void Camera::onMaxPacketsDelayViolated() {
    m_liveRTSPStreamStatusNotifier.onRTSPStreamError( m_streamId, StreamError::CAMERA_PACKET_DELAY );
}

bool Camera::startStreamWithDifferentTransport() {
    Log::debug( cc::debug( "Try to start stream: " ) + cc::attention( m_streamId ) );
    for( const auto& rtspTransport : m_rtspTransports ) {
        if( m_cameraInput->start( rtspTransport ) ) {
            return true;
        }
        if( m_cameraInput->getError() != "Protocol not supported" ) {
            break;
        }
    }
    m_liveRTSPStreamStatusNotifier.onRTSPStreamError( m_streamId, StreamError::CAMERA_UNREACHABLE );
    return false;
}

void Camera::onStreamReadingStop() {
    LOGGER_DEBUG( cc::debug( "Stream reading stopped: " ) + cc::note( m_streamId ) );
    std::unique_lock< std::recursive_mutex > guard( m_methodMutex );

    if( m_cloudStreamSaver->isSavingCreated() ) {
        m_cloudStreamSaver->stopRecording();
    }

    if( m_particleRecorder->isMakingRecord() ) {
        m_particleRecorder->stopMakingRecord();
        m_particleRecordingWasForceStopped = true;
    }

    if( m_liveRTSPStreamStatus == LiveRTSPStreamStatus::RUNNING ) {
        m_liveRTSPStreamStatus = LiveRTSPStreamStatus::PAUSED;
        m_liveRTSPStreamStatusNotifier.onRTSPStreamStopped( m_streamId );
    }

    guard.unlock();

    m_surpriseTriggeringBlocker = true;
    m_startingCondVar.notify_all();
}

}  // namespace HubPlatform
