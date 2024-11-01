#pragma once

#include "cameras/cloud/cloud_stream_saver_interface.h"
#include "cameras/input_output/camera_archiver.h"

namespace HubPlatform {

class ICameraInput;

class CloudStreamSaver : public ICloudStreamSaver {
public:
    using VideoPath = std::string;
    using StreamId = std::string;

    CloudStreamSaver( ICameraInput& cameraInput, StreamId streamId, VideoPath videoPath );

    void createRecording( int segmentDurationSec,
                          int64_t secondsBeforeStart,
                          int segmentsLimit,
                          const std::string& format,
                          SaveFileCallback saveFile ) override;
    void deleteRecording() override;
    bool isSavingCreated() const override;

    void startRecording( const Json& metaData, SaveFileCallback saveFile ) override;
    void startRecording() override;
    void stopRecording() override;
    bool isStarted() const override;

private:
    CameraArchiverUniquePtr m_cameraArchiver;
    ICameraInput& m_cameraInput;
    VideoPath m_videoPath;
    StreamId m_streamId;
    bool m_isStarted = false;
};

}  // namespace HubPlatform
