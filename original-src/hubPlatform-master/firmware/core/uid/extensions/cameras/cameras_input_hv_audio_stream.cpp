#include "uid/extensions/cameras/cameras_input_hv_audio_stream.h"

#include "common_utils/cpp14.h"
#include "tiny/tiny_utils.h"

#include <unistd.h>
#include <poll.h>

namespace HubPlatform {
namespace Cameras {

InputHVAudioStream::InputHVAudioStream( AudioStreamInfo info,
                                        StreamEventsHandler& streamEventsHandler,
                                        Services::Service& hostService )
: InputAudioStream( std::move( info ), streamEventsHandler, hostService )
, m_curl( nullptr )
, m_socket( 0 )
, m_running( false )
, m_shouldStop( false ) {}

InputHVAudioStream::~InputHVAudioStream() {
    stop();
}

const RequestHVInfo& InputHVAudioStream::getRequestInfo() const {
    return *getInfo().requestInfo.hv;
}

void InputHVAudioStream::start( const std::weak_ptr< InputAudioStream >& weakSelf ) {
    LOGGER_TRACE_RAW( cc::trace( "Starting input audio stream " ) + cc::note( getInfo().streamId ) );
    if( m_running ) {
        emitStreamError( std::string( "Stream already started" ) );
        return;
    }

    if( m_shouldStop ) {
        emitStreamError( std::string( "Stream already stopped" ) );
        return;
    }

    // Init request HEAD
    std::string head;
    auto& url = getRequestInfo().url;
    try {
        tiny::url parsedUrl( url );

        head += getRequestInfo().getTypeString() + " " + parsedUrl.path() + " HTTP/1.1\r\n";
        head += "Host: " + parsedUrl.host() + ":" + parsedUrl.port() + "\r\n";
        if( !getRequestInfo().user.empty() && !getRequestInfo().password.empty() ) {
            auto authData = getRequestInfo().user + ":" + getRequestInfo().password;
            authData = tiny::tools::base64::encode( reinterpret_cast< const unsigned char* >( authData.c_str() ),
                                                    authData.size() );
            head += "Authorization: Basic " + authData + "\r\n";
        }
        head += "Accept: */*\r\n";
        for( auto& header : getRequestInfo().headers ) {
            head += header.first + ": " + header.second + "\r\n";
        }
        head += "\r\n";
    }
    catch( const tiny::url::parse_error& error ) {
        emitStreamError( error.what() );
        return;
    }

    // Connect
    m_curl = curl_easy_init();
    curl_easy_setopt( m_curl, CURLOPT_URL, url.c_str() );
    curl_easy_setopt( m_curl, CURLOPT_NOSIGNAL, 1 );
    curl_easy_setopt( m_curl, CURLOPT_CONNECT_ONLY, 1L );
    curl_easy_setopt( m_curl, CURLOPT_NOSIGNAL, 1L );

    CURLcode res = curl_easy_perform( m_curl );
    if( res != CURLE_OK ) {
        emitStreamError( std::string( "Error connecting to camera: " ) + curl_easy_strerror( res ) );
        return;
    }
    /* Extract the socket from the curl handle - we'll need it for waiting. */
    res = curl_easy_getinfo( m_curl, CURLINFO_ACTIVESOCKET, &m_socket );

    if( res != CURLE_OK ) {
        emitStreamError( std::string( "Error getting camera stream: " ) + curl_easy_strerror( res ) );
        return;
    }

    m_running = true;

    // Send HEAD
    LOGGER_TRACE_RAW( cc::trace( "Head: " ) + cc::note( head ) );
    send( head.c_str(), head.size() );

    // Start response reading thread
    m_workThread = std::thread( &InputHVAudioStream::work, this );
}

bool InputHVAudioStream::isRunning() const {
    return m_running && !m_shouldStop;
}

bool InputHVAudioStream::send( const char* data, size_t size ) {
    if( !isRunning() ) {
        emitStreamError( std::string( "Error sending data: Stream not running" ) );
        return false;
    }

    std::string error;
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
        stop();
        return false;
    }

    return true;
}

void InputHVAudioStream::stop() {
    if( !m_shouldStop ) {
        LOGGER_TRACE_RAW( cc::trace( "Stopping input audio stream " ) + cc::note( getInfo().streamId ) );
    }
    m_shouldStop = true;
    if( m_workThread.joinable() ) {
        m_workThread.join();
    }
    curl_easy_cleanup( m_curl );
    m_curl = nullptr;
    m_socket = 0;
}

void InputHVAudioStream::work() {
    emitStreamStarted();
    std::string error;
    std::string response;
    int responseCode = 0;
    while( !m_shouldStop ) {
        char buffer[ 1024 ];

        struct pollfd pollFdArray = { m_socket, POLLIN, 0 };

        int numEvents = poll( &pollFdArray, 1, 100 );
        if( numEvents < 0 ) {
            if( errno == EINTR ) {
                continue;
            }
            error = std::string( "Error waiting for new data: " ) + strerror( errno );
            break;
        }
        else if( numEvents == 0 ) {
            continue;
        }

        auto bytes = read( m_socket, buffer, 1024 );
        if( bytes == 0 ) {
            break;
        }
        else if( bytes < 0 ) {
            if( errno == EAGAIN || errno == EWOULDBLOCK ) {
                continue;
            }
            else {
                error = std::string( "Error receiving data: " ) + strerror( errno );
                break;
            }
        }
        else {
            if( !responseCode ) {
                response.append( buffer, bytes );
                auto end = response.find( "\r\n" );
                if( end != std::string::npos ) {
                    response.resize( end );
                    auto pos = response.find( ' ' );
                    responseCode = atoi( &response[ pos ] );
                    if( responseCode != 200 ) {
                        error = std::string( "Response code: " ) + std::to_string( responseCode );
                        break;
                    }
                }
            }
        }
    }

    m_running = false;
    if( !error.empty() ) {
        emitStreamError( error );
    }
    else {
        emitStreamStopped();
    }
}

bool InputHVAudioStream::stopDataSending() {
    stop();
    return InputAudioStream::stopDataSending();
}

}  // namespace Cameras
}  // namespace HubPlatform
