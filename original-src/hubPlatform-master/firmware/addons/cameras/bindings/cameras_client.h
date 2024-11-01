#pragma once

#include "uid/extensions/cameras/cameras_audio_stream_info.h"
#include "service_interfaces/script_client.h"
#include "addon_interfaces/addon.h"

namespace HubPlatform {

class CamerasClient : public ScriptClient {
public:
    explicit CamerasClient( Addons::Addon& hostAddon );

    JsonObject addCameraStream( const std::string& pluginId,
                                const std::string& streamId,
                                const std::string& streamName,
                                const std::string& rtspUrl,
                                const std::string& description );
    JsonObject removeCameraStream( const std::string& streamId );

    void createLiveRtspStream( const std::string& streamId );
    void deleteLiveRtspStream( const std::string& streamId );

    void createStreamSavingToCloud( const std::string& streamId,
                                    int segmentDurationSec,
                                    int secondsBeforeStart,
                                    int segmentsLimit,
                                    const std::string& format );
    void deleteStreamSavingToCloud( const std::string& streamId );
    void startStreamSavingToCloud( const std::string& streamId, const Json& metaData, bool makeVideoParticle );
    void stopStreamSavingToCloud( const std::string& streamId );

    void setVideoParticleSettings( const std::string& pluginId,
                                   int preRecordDurationSec,
                                   int recordDurationSec,
                                   int resolutionDivisor,
                                   const std::string& format );

    void makeSnapshot( const std::string& streamId, const std::string& snapshotId, const Json& metaData );

    void createAudioStream( const Cameras::AudioStreamInfo& info );
    void deleteAudioStream( const std::string& streamId );
    JsonObject getAudioStreamInfo( const std::string& streamId );
    JsonObject getStreamInfo( const std::string& streamId );
};
}  // namespace HubPlatform
