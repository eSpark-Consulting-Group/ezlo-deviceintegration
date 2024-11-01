#pragma once

#include "cameras/input_output/camera_input_interface.h"

#include <string>
#include <functional>
#include <thread>
#include <atomic>

class AVPacket;

namespace HubPlatform {

namespace UT {
class CameraInputTestSuite;
}

class AVPacketWrapper;

class CameraInput final : public ICameraInput {
public:
    using StreamIndex = int;
    using Dts = int64_t;

    CameraInput( PacketHandlerSharedPtr packetHandler, std::string url );
    ~CameraInput() override;

    ///
    /// \brief calls initStreams and starts reading configured streams from camera in separate thread.
    /// Does nothing if reading was started and is in progress.
    /// \param rtspTransport Sets stream reading protocol
    /// \param onStreamReadingStop callback
    /// \return true if reading thread was started
    ///
    bool start( RtspTransport rtspTransport = RtspTransport::Default ) override;

    /// \brief pause of packet streaming
    void pause() override;

    /// \brief start packet streaming after pause. Doesn't start streaming if it was stopped
    void run() override;

    /// \brief stop packet streaming
    void stop() override;

    bool isRunning() const override;

    const AVStream* getVideoStream() const override;
    const AVStream* getAudioStream() const override;
    const std::string& getSdp() const override;
    int getVideoIndex() const override;
    const AVCodecContext* getInputCodecContext() const override;

    const PacketHandlerSharedPtr& getPacketHandler() const override;
    std::string getError() const override;

    void setEventsListener( ICameraInputEventsListener* listener ) override;

    ///
    /// \brief initStreams Initializes input streams. For manual intialization.
    ///         start() do it automatically. Do not use use along start() method
    /// \param rtspTransport Sets stream reading protocol
    /// \return ffmpeg error code, negative means error
    ///
    int initStreams( RtspTransport rtspTransport );

    /// \brief read next packet. Do not use use along start() method
    int readNextPacket( AVPacket& packet );

private:
    const AVStream* getStream( int avMediaType ) const;
    void closeStream();
    void extractSdp();

    std::chrono::microseconds getDurationBetweenPackets( const AVPacket& packet );

private:
    std::thread m_worker;
    std::atomic_bool m_stopped{ true };
    std::atomic_bool m_isRunning{ false };
    std::atomic_bool m_isPaused{ false };
    std::atomic_bool m_isReceivedKeyFrame{ false };

    PacketHandlerSharedPtr m_packetHandler;
    const std::string m_url;
    std::string m_sdp;

    AVFormatContext* m_inputFormatContext = nullptr;
    AVCodecContext* m_codecContext = nullptr;
    int m_videoStreamIndex = 0;
    std::map< StreamIndex, Dts > m_lastDts;

    ICameraInputEventsListener* m_listener = nullptr;

    std::chrono::milliseconds m_maxDelayBetweenPackets;
    std::chrono::milliseconds m_packetsProcessingDelay;
    std::chrono::milliseconds m_accumulatedDelayThreshold;

    std::string m_error;

    int m_timeBase;

    friend class UT::CameraInputTestSuite;
};

}  // namespace HubPlatform
