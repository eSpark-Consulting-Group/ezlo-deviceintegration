#pragma once

#include "uid/extensions/cameras/cameras_stream_events_handler.h"
#include "uid/extensions/cameras/cameras_input_audio_stream.h"

class IDbClient;

namespace HubPlatform {

namespace Services {
class Service;
}

namespace Cameras {

class AudioStreamManager {
public:
    explicit AudioStreamManager( StreamEventsHandler& streamEventsHandler, Services::Service& hostService );

    InputAudioStream* createInputAudioStream( AudioStreamInfo audioStreamInfo );
    void deleteInputAudioStream( const std::string& streamId );
    InputAudioStream* getInputAudioStream( const std::string& streamId );

private:
    std::shared_ptr< InputAudioStream > constructInputAudioStream( AudioStreamInfo audioStreamInfo );
    void streamsInterruptedNotify();

private:
    StreamEventsHandler& m_streamEventsHandler;
    Services::Service& m_hostService;
    IDbClient& m_dbClient;

    std::unordered_map< std::string, std::shared_ptr< InputAudioStream > > m_inputStreams;
};

}  // namespace Cameras
}  // namespace HubPlatform
