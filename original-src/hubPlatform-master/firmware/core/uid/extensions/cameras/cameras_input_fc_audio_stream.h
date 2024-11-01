#pragma once

#include "uid/extensions/cameras/cameras_input_audio_stream.h"
#include "service_interfaces/service.h"

#include "system_utils/file_descriptor.h"

#include <cstring>
#include <utility>

namespace HubPlatform {

namespace Services {
class Service;
}

namespace Cameras {

class InputFCAudioStream final : public InputAudioStream {
public:
    InputFCAudioStream( AudioStreamInfo info,
                        StreamEventsHandler& streamEventsHandler,
                        Services::Service& hostService );
    ~InputFCAudioStream() override;

    bool isRunning() const override;
    bool send( const char* data, size_t size ) override;
    void start( const std::weak_ptr< InputAudioStream >& weakSelf ) override;
    void stop() override;

    bool startDataSending() override;
    bool stopDataSending() override;

private:
    const RequestFCInfo& getRequestInfo() const;
    bool sendOpenTalk();
    bool sendCloseTalk();
    bool sendMessage();

    std::pair< bool, System::FileDescriptor > checkConnection();
    bool connect();
    void clean();

    void cameraAliveCheckingJob();

private:
    enum class MessageId : uint32_t {
        OpenTalk = 20001,
        CloseTalk,  // 20002
        TalkFrame,  // 20003
    };

#pragma pack( 1 )
    struct Message {
        MessageId id;
        uint32_t sessionId;
        char magic[ 5 ];
        uint32_t dataLen;
        char data[ 960 ];
        char userName[ 64 ];
        char password[ 64 ];

        void clear() {
            memset( this, 0x00, size() );
        }

        const char* buffer() const {
            return reinterpret_cast< const char* >( this );
        }

        static constexpr size_t size() {
            return sizeof( Message );
        }

        static constexpr size_t dataSize() {
            return sizeof( data );
        }
    };
#pragma pack()

private:
    std::atomic_bool m_running;
    std::atomic_bool m_shouldStop;

    static uint32_t m_sessionIdCounter;

    Message m_message;
    size_t m_preparedDataSize;
    System::FileDescriptor m_socket;

    Services::PostponedTaskPtr m_task;
};

}  // namespace Cameras
}  // namespace HubPlatform
