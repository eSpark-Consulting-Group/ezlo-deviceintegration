#include "uid/extensions/cameras/cameras_input_audio_stream.h"
#include "service_interfaces/service.h"

namespace HubPlatform {
namespace Cameras {

InputAudioStream::InputAudioStream( AudioStreamInfo info,
                                    StreamEventsHandler& streamEventsHandler,
                                    Services::Service& hostService )
: m_streamInfo( std::move( info ) )
, m_streamEventsHandler( streamEventsHandler )
, m_hostService( hostService )
, m_dataSending( false ) {}

const AudioStreamInfo& InputAudioStream::getInfo() const {
    return m_streamInfo;
}

void InputAudioStream::emitStreamStarted() {
    m_streamEventsHandler.onStreamStarted( m_streamInfo.streamId, m_streamInfo.userData );
}

void InputAudioStream::emitStreamStopped() {
    m_streamEventsHandler.onStreamStopped( m_streamInfo.streamId, m_streamInfo.userData );
}

void InputAudioStream::emitStreamError( const std::string& errorMessage ) {
    m_streamEventsHandler.onStreamError( m_streamInfo.streamId, m_streamInfo.userData, errorMessage );
}

Services::Service& InputAudioStream::getHostService() {
    return m_hostService;
}

Services::TaskLoop& InputAudioStream::getHostTaskLoop() {
    return m_hostService;
}

bool InputAudioStream::startDataSending() {
    if( m_dataSending.exchange( true ) ) {
        LOGGER_DEBUG_RAW( cc::debug( "Trying to start sending data on stream that is already sending data" ) );
        return false;
    }
    return true;
}

bool InputAudioStream::stopDataSending() {
    if( !m_dataSending.exchange( false ) ) {
        LOGGER_DEBUG_RAW( cc::debug( "Trying to stop sending data on stream that is not sending data" ) );
        return false;
    }
    return true;
}

}  // namespace Cameras
}  // namespace HubPlatform
