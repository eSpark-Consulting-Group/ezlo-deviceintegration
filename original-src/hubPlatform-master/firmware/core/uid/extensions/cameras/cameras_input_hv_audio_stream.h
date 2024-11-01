#pragma once

#include "uid/extensions/cameras/cameras_input_audio_stream.h"

#include <curl/curl.h>
#include <thread>
#include <cstring>

namespace HubPlatform {

namespace Services {
class Service;
}

namespace Cameras {

class InputHVAudioStream final : public InputAudioStream {
public:
    InputHVAudioStream( AudioStreamInfo info,
                        StreamEventsHandler& streamEventsHandler,
                        Services::Service& hostService );
    ~InputHVAudioStream() override;

    bool isRunning() const override;
    bool send( const char* data, size_t size ) override;
    void start( const std::weak_ptr< InputAudioStream >& weakSelf ) override;
    void stop() override;

    bool stopDataSending() override;

private:
    void work();
    const RequestHVInfo& getRequestInfo() const;

private:
    CURL* m_curl;
    curl_socket_t m_socket;
    std::thread m_workThread;
    std::atomic_bool m_running;
    std::atomic_bool m_shouldStop;
};

}  // namespace Cameras
}  // namespace HubPlatform
