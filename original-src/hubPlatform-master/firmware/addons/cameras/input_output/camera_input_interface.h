#pragma once

#include "cameras/defs.h"
#include "cameras/input_output/camera_input_events_listener_interface.h"

#include <functional>

struct AVFormatContext;
struct AVStream;
struct AVCodecContext;

namespace HubPlatform {

class ICameraInput {
public:
    enum class RtspTransport { Default /* means choice by ffmpeg (most likely udp) */, Tcp, Udp };

    ///
    /// \brief calls initStreams and starts reading configured streams from camera in separate thread.
    /// Does nothing if reading was started and is in progress.
    /// \param rtspTransport Sets stream reading protocol
    /// \param onStreamReadingStop callback
    /// \return true if reading thread was started
    ///
    virtual bool start( RtspTransport rtspTransport ) = 0;

    /// \brief pause of packet streaming
    virtual void pause() = 0;

    /// \brief start packet streaming after pause. Doesn't start streaming if it was stopped
    virtual void run() = 0;

    /// \brief stop packet streaming
    virtual void stop() = 0;

    virtual bool isRunning() const = 0;

    virtual const AVStream* getVideoStream() const = 0;
    virtual const AVStream* getAudioStream() const = 0;
    virtual const std::string& getSdp() const = 0;
    virtual int getVideoIndex() const = 0;
    virtual const AVCodecContext* getInputCodecContext() const = 0;

    virtual const PacketHandlerSharedPtr& getPacketHandler() const = 0;
    virtual std::string getError() const = 0;

    virtual void setEventsListener( ICameraInputEventsListener* listener ) = 0;

    virtual ~ICameraInput() = default;
};

}  // namespace HubPlatform
