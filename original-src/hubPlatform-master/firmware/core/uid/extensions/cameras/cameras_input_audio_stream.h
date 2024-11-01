#pragma once

#include "uid/extensions/cameras/cameras_audio_stream_info.h"
#include "uid/extensions/cameras/cameras_stream_events_handler.h"

namespace HubPlatform {

namespace Services {
class Service;
class TaskLoop;
}  // namespace Services

namespace Cameras {

class InputAudioStream {
public:
    InputAudioStream( AudioStreamInfo info, StreamEventsHandler& streamEventsHandler, Services::Service& hostService );
    virtual ~InputAudioStream() = default;

    const AudioStreamInfo& getInfo() const;

    virtual bool isRunning() const = 0;
    virtual bool send( const char* data, size_t size ) = 0;
    virtual void start( const std::weak_ptr< InputAudioStream >& weakSelf ) = 0;
    virtual void stop() = 0;

    virtual bool startDataSending();
    virtual bool stopDataSending();

protected:
    void emitStreamStarted();
    void emitStreamStopped();
    void emitStreamError( const std::string& errorMessage );

    Services::Service& getHostService();
    Services::TaskLoop& getHostTaskLoop();

private:
    AudioStreamInfo m_streamInfo;
    StreamEventsHandler& m_streamEventsHandler;
    Services::Service& m_hostService;

    std::atomic_bool m_dataSending;
};

}  // namespace Cameras
}  // namespace HubPlatform
