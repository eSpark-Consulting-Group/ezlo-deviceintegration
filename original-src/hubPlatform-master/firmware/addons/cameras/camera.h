#pragma once
#include "cameras/defs.h"
#include "cameras/input_output/camera_input_events_listener_interface.h"
#include "cameras/input_output/camera_input_interface.h"
#include "cameras/input_output/camera_snapshoter.h"
#include "cameras/input_output/resolution.h"
#include "json_utils/json_fwd.h"

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <map>

namespace HubPlatform {

namespace Services {
class TaskLoop;
}

namespace UT {
class CameraTestSuite;
}

struct StreamParticleSettings;

class ICameraInput;
class ICloudStreamSaver;
class IPostponedMethodManager;
class ILiveStreamer;
class IStreamParticleRecorder;
class ISnapshotManager;
class ILiveRTSPStreamStatusNotifier;

class Camera : public ICameraInputEventsListener {
public:
    using FilePath = std::string;
    using RtspTransports = std::vector< ICameraInput::RtspTransport >;
    struct Paths {
        std::string videos;
        std::string snapshots;
    };

    Camera( std::string streamId,
            std::string ipAddress,
            std::unique_ptr< ICameraInput > cameraInput,
            std::unique_ptr< IPostponedMethodManager > postponedMethodManager,
            std::unique_ptr< ILiveStreamer > liveStreamer,
            std::unique_ptr< ICloudStreamSaver > cloudStreamSaver,
            std::unique_ptr< IStreamParticleRecorder > particleRecorder,
            std::unique_ptr< ISnapshotManager > snapshotManager,
            ILiveRTSPStreamStatusNotifier& liveRTSPStreamStatusNotifier,
            RtspTransports tryConnectWithProtocols = RtspTransports{ ICameraInput::RtspTransport::Tcp } );

    ~Camera();

    void startStream();

    void createLiveRtspStream();
    void deleteLiveRtspStream();

    void createStreamSavingToCloud( int segmentDurationSec,
                                    int64_t secondsBeforeStart,
                                    int segmentsLimit,
                                    const std::string& format,
                                    const SaveFileCallback& saveFile );
    void deleteStreamSavingToCloud();
    void startStreamSavingToCloud( const Json& metaData, const SaveFileCallback& saveFile );
    void stopStreamSavingToCloud();

    void createStreamParticleRecording( const StreamParticleSettings& settings, const SaveFileCallback& saveFile );
    void deleteStreamParticleRecording();
    void recordStreamParticle( const Json& metaData );
    void stopStreamParticleRecording();
    bool isRecordingStreamParticle() const noexcept;
    bool isRunning() const;

    void makeSnapshotForCloud( const Json& metaData,
                               SaveFileCallback saveFile,
                               const std::string& snapshotId,
                               const std::function< void( const Json& storageKey ) >& onFinished,
                               const std::function< void( const std::string& errorDesc ) >& onError );

    void makeSnapshotForUI( std::shared_ptr< Resolution > resolution,
                            const std::string& requestId,
                            const std::function< void( const Json& storageKey ) >& response );

    std::string getLiveRtspStreamLink() const;
    void setLiveRtspStreamLink( const std::string& link );

    std::string getStreamId() const;

private:
    void onInputReadingStopped( int errorCode ) override;
    void onMaxPacketsDelayViolated() override;
    void onAccumulatedDelayThresholdReached() override;

    bool startStreamWithDifferentTransport();
    void onStreamReadingStop();

private:
    const std::string m_streamId;

    enum class LiveRTSPStreamStatus { STOPPED, RUNNING, PAUSED };
    LiveRTSPStreamStatus m_liveRTSPStreamStatus = LiveRTSPStreamStatus::STOPPED;

    std::unique_ptr< IPostponedMethodManager > m_postponedMethodManager;
    std::unique_ptr< ILiveStreamer > m_liveStreamer;
    std::unique_ptr< ICloudStreamSaver > m_cloudStreamSaver;
    std::unique_ptr< IStreamParticleRecorder > m_particleRecorder;
    std::unique_ptr< ISnapshotManager > m_snapshotManager;

    bool m_particleRecordingWasForceStopped = false;

    std::string m_liveRtspStreamLink;
    std::thread m_startStreamThread;
    std::atomic_bool m_isStartStreamCanRun{ true };
    std::atomic_bool m_surpriseTriggeringBlocker{ false };
    std::recursive_mutex m_methodMutex;
    std::mutex m_startingMutex;
    std::condition_variable m_startingCondVar;
    std::mutex m_streamRestartingMutex;
    std::condition_variable m_streamRestartingCondVar;

    std::unique_ptr< ICameraInput > m_cameraInput;

    ILiveRTSPStreamStatusNotifier& m_liveRTSPStreamStatusNotifier;

    std::chrono::seconds m_startStreamTriesSleepTime = DefaultStartStreamTriesSleepTime;

    const RtspTransports m_rtspTransports;

    static constexpr std::chrono::seconds DefaultStartStreamTriesSleepTime = std::chrono::seconds( 10 );

    friend class UT::CameraTestSuite;
};
}  // namespace HubPlatform
