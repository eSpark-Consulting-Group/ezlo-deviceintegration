#include "uid/extensions/cameras/cameras_input_fc_audio_stream.h"

#include "common_utils/cpp14.h"
#include "service_interfaces/service.h"

#include <chrono>
#include <tuple>

#include <unistd.h>
#include <poll.h>
#include <netinet/in.h>
#include <uv.h>

namespace HubPlatform {
namespace Cameras {

namespace {}  // namespace

uint32_t InputFCAudioStream::m_sessionIdCounter = 0;

InputFCAudioStream::InputFCAudioStream( AudioStreamInfo info,
                                        StreamEventsHandler& streamEventsHandler,
                                        Services::Service& hostService )
: InputAudioStream( std::move( info ), streamEventsHandler, hostService )
, m_running( false )
, m_shouldStop( false )
, m_preparedDataSize( 0 ) {}

InputFCAudioStream::~InputFCAudioStream() {
    stop();
}

const RequestFCInfo& InputFCAudioStream::getRequestInfo() const {
    return *getInfo().requestInfo.fc;
}

void InputFCAudioStream::start( const std::weak_ptr< InputAudioStream >& weakSelf ) {
    LOGGER_TRACE_RAW( cc::trace( "Starting input audio stream " ) + cc::note( getInfo().streamId ) );
    if( m_running ) {
        emitStreamError( std::string( "Stream already started" ) );
        return;
    }

    if( m_shouldStop ) {
        emitStreamError( std::string( "Stream already stopped" ) );
        return;
    }

    // Try to connect to check if host is reachable and port is open
    auto ws = weakSelf;

    m_task = getHostTaskLoop().createPostponedTask( [ws]( Services::PostponedTask& ) {
        auto stream = ws.lock();
        if( !stream ) {
            LOGGER_WARN_RAW( cc::warn( "Input audio stream already removed before scheduleRun" ) );
            return;
        }

        static_cast< InputFCAudioStream& >( *stream.get() ).cameraAliveCheckingJob();
    } );
    m_task->start( std::chrono::seconds( 0 ), std::chrono::seconds( 30 ) );
}

bool InputFCAudioStream::isRunning() const {
    return m_running && !m_shouldStop;
}

void InputFCAudioStream::cameraAliveCheckingJob() {
    bool connected;
    std::tie( connected, std::ignore ) = checkConnection();
    if( connected ) {
        if( !m_running ) {
            m_running = true;

            LOGGER_TRACE_RAW( cc::trace( "Resuming input audio stream " ) + cc::note( getInfo().streamId ) );
            emitStreamStarted();
        }
    }
    else {
        if( m_running ) {
            m_running = false;

            LOGGER_TRACE_RAW( cc::trace( "Interrupting input audio stream " ) + cc::note( getInfo().streamId ) );
            emitStreamError( std::string( "Error checking camera: Camera is not reachable" ) );
        }
    }
}

bool InputFCAudioStream::send( const char* data, size_t size ) {
    if( !isRunning() ) {
        emitStreamError( std::string( "Error sending data: Stream not running" ) );
        return false;
    }

    while( size ) {
        size_t prepare = std::min( Message::dataSize() - m_preparedDataSize, size );
        // Prepare data
        memcpy( m_message.data + m_preparedDataSize, data, prepare );
        m_preparedDataSize += prepare;

        // Send prepared data
        if( m_preparedDataSize == Message::dataSize() ) {
            m_preparedDataSize = 0;
            m_message.id = MessageId::TalkFrame;
            m_message.dataLen = Message::size();
            if( !sendMessage() ) {
                return false;
            }
        }

        size -= prepare;
        data += prepare;
    }

    return true;
}

void InputFCAudioStream::stop() {
    m_task->stop();
    if( !m_shouldStop ) {
        LOGGER_TRACE_RAW( cc::trace( "Stopping input audio stream " ) + cc::note( getInfo().streamId ) );
        emitStreamStopped();
    }
    m_shouldStop = true;
    clean();
}

void InputFCAudioStream::clean() {
    close( m_socket.release() );
}

bool InputFCAudioStream::connect() {
    bool connected;
    std::tie( connected, m_socket ) = checkConnection();

    return connected;
}

std::pair< bool, System::FileDescriptor > InputFCAudioStream::checkConnection() {
    sockaddr_in addr;
    int sock;

    std::memset( &addr, 0, sizeof( addr ) );

    if( ( sock = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 ) {
        emitStreamError( "Failed to create a socket" );
        return { false, System::FileDescriptor() };
    }

    System::FileDescriptor socketFd( sock );

    addr.sin_family = AF_INET;
    addr.sin_port = htons( getRequestInfo().port );
    addr.sin_addr.s_addr = inet_addr( getRequestInfo().address.c_str() );
    if( addr.sin_addr.s_addr == INADDR_NONE ) {
        emitStreamError( "Incorrect request address" );
        return { false, System::FileDescriptor() };
    }

    auto res = ::connect( socketFd, reinterpret_cast< sockaddr* >( &addr ), sizeof( addr ) );
    if( res < 0 ) {
        emitStreamError( std::string( "Error connecting to camera: " ) + strerror( errno ) );
        return { false, System::FileDescriptor() };
    }

    return { true, std::move( socketFd ) };
}

bool InputFCAudioStream::startDataSending() {
    if( !InputAudioStream::startDataSending() ) {
        return false;
    }

    if( !connect() ) {
        return false;
    }

    return sendOpenTalk();
}

bool InputFCAudioStream::stopDataSending() {
    if( !InputAudioStream::stopDataSending() ) {
        return false;
    }

    auto res = sendCloseTalk();
    clean();

    return res;
}

bool InputFCAudioStream::sendOpenTalk() {
    m_preparedDataSize = 0;
    m_message.clear();
    m_message.sessionId = m_sessionIdCounter++;
    memcpy( m_message.magic,
            getRequestInfo().magic.c_str(),
            std::min( getRequestInfo().magic.size(), sizeof( m_message.magic ) ) );
    memcpy( m_message.userName,
            getRequestInfo().user.c_str(),
            std::min( getRequestInfo().user.size(), sizeof( m_message.userName ) ) );
    memcpy( m_message.password,
            getRequestInfo().password.c_str(),
            std::min( getRequestInfo().password.size(), sizeof( m_message.password ) ) );

    m_message.id = MessageId::OpenTalk;
    return sendMessage();
}

bool InputFCAudioStream::sendCloseTalk() {
    m_message.id = MessageId::CloseTalk;
    return sendMessage();
}

bool InputFCAudioStream::sendMessage() {
    std::string error;
    auto size = Message::size();
    const char* data = m_message.buffer();

    while( size ) {
        struct pollfd pollFdArray = { m_socket, POLLOUT, 0 };
        int numEvents = poll( &pollFdArray, 1, 1000 );
        if( numEvents < 0 ) {
            if( errno == EINTR ) {
                continue;
            }
            error = std::string( "Error waiting for possibility to send data: " ) + strerror( errno );
            break;
        }
        else if( numEvents == 0 ) {
            error = std::string( "Timeout waiting for possibility to send data" );
            break;
        }

        auto bytes = write( m_socket, data, size );
        if( bytes < 0 ) {
            error = std::string( "Error sending data: " ) + strerror( errno );
            break;
        }
        size -= bytes;
        data += bytes;
    }

    if( !error.empty() ) {
        emitStreamError( error );
        clean();
        return false;
    }

    return true;
}

}  // namespace Cameras
}  // namespace HubPlatform
