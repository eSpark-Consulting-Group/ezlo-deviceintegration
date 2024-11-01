#pragma once

#include "cameras/local_stream/live_streamer_interface.h"
#include "cameras/packet/packet_publisher_interface.h"

#include <memory>

namespace HubPlatform {

class ICameraInput;

namespace Cameras {
class IRtspServerController;

class LiveStreamer : public ILiveStreamer {
public:
    using StreamId = std::string;

    LiveStreamer( IRtspServerController& rtspServer, ICameraInput& cameraInput, StreamId streamId );

    void createCameraStream() override;

    void deleteCameraStream() override;

private:
    IRtspServerController& m_rtspServer;
    ICameraInput& m_cameraInput;
    std::unique_ptr< IPacketPublisher > m_packetPublisher;
    StreamId m_streamId;
};

}  // namespace Cameras

}  // namespace HubPlatform
