#pragma once

#include "cameras/cloud/stream_particle_recorder_interface.h"
#include "cameras/input_output/resolution.h"
#include "cameras/packet/packet_publisher.h"
#include "cameras/defs.h"

namespace HubPlatform {

class ICameraInput;

class StreamParticleRecorder : public IStreamParticleRecorder {
public:
    using RecordsDir = std::string;
    using StreamId = std::string;

    StreamParticleRecorder( ICameraInput& cameraInput, StreamId streamId, RecordsDir recordsDir );
    ~StreamParticleRecorder() override;

    void createRecording( const StreamParticleSettings& settings, SaveFileCallback saveFile ) override;
    void deleteRecording() override;
    void makeRecord( const Json& metaData ) override;
    void makeRecord() override;
    void stopMakingRecord() override;

    bool isRecordingCreated() const override;
    bool isMakingRecord() const override;

private:
    CameraOutputUniquePtr createCameraOutput( StorageSharedPtr storage, const StreamParticleSettings& settings ) const;

    static Resolution determineOutputResolution( const AVStream& sourceVideoStream, uint32_t divisor );

private:
    ICameraInput& m_cameraInput;
    PacketHandler::ListenerId m_listenerId;

    CameraOutputUniquePtr m_cameraOutput;
    std::chrono::seconds m_recordDuration;

    RecordsDir m_recordsDir;
    StreamId m_streamId;
};

}  // namespace HubPlatform
