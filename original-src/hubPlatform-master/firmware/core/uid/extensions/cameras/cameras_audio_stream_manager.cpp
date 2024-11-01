#include "uid/extensions/cameras/cameras_audio_stream_manager.h"
#include "uid/extensions/cameras/cameras_input_hv_audio_stream.h"
#include "uid/extensions/cameras/cameras_input_fc_audio_stream.h"

#include "database_client/dbqueryparser.h"
#include "service_interfaces/service.h"
#include "database_client/idbclient.h"
#include "json_utils/json_utils.h"
#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace Cameras {

AudioStreamManager::AudioStreamManager( StreamEventsHandler& streamEventsHandler, Services::Service& hostService )
: m_streamEventsHandler( streamEventsHandler )
, m_hostService( hostService )
, m_dbClient( m_hostService.getDBClient() ) {
    // TODO: maybe we should restore streams?
    streamsInterruptedNotify();
}

InputAudioStream* AudioStreamManager::createInputAudioStream( AudioStreamInfo audioStreamInfo ) {
    const auto streamId = audioStreamInfo.streamId;
    const auto userData = audioStreamInfo.userData;
    LOGGER_DEBUG_RAW( cc::debug( "Creating input audio stream: " ) + cc::note( streamId ) );
    auto streamPtr = getInputAudioStream( streamId );
    if( streamPtr ) {
        m_streamEventsHandler.onStreamError( streamId, userData, "Stream with specified id already exist" );
        return nullptr;
    }

    auto audioStream = constructInputAudioStream( std::move( audioStreamInfo ) );
    if( !audioStream ) {
        m_streamEventsHandler.onStreamError( streamId, userData, "Failed to create stream with specified parameters" );
        return nullptr;
    }
    auto it = m_inputStreams.insert( std::make_pair( streamId, audioStream ) );
    streamPtr = it.first->second.get();
    streamPtr->start( audioStream );

    m_dbClient.insert( UiExtensionsCamerasDBName, JSON( db::ID_FIELD_NAME << streamId << "userData" << userData ) );

    return streamPtr;
}

void AudioStreamManager::deleteInputAudioStream( const std::string& streamId ) {
    LOGGER_DEBUG_RAW( cc::debug( "Deleting input audio stream: " ) + cc::note( streamId ) );
    if( m_inputStreams.count( streamId ) == 0 ) {
        return;
    }
    m_inputStreams.erase( streamId );

    m_dbClient.remove( UiExtensionsCamerasDBName, JSON( db::ID_FIELD_NAME << streamId ) );
}

InputAudioStream* AudioStreamManager::getInputAudioStream( const std::string& streamId ) {
    if( m_inputStreams.count( streamId ) == 0 ) {
        return nullptr;
    }
    return m_inputStreams[ streamId ].get();
}

std::shared_ptr< InputAudioStream > AudioStreamManager::constructInputAudioStream( AudioStreamInfo audioStreamInfo ) {
    switch( audioStreamInfo.requestInfo.format ) {
    case RequestInfo::Format::Hikvision:
        return std::make_shared< InputHVAudioStream >(
            std::move( audioStreamInfo ), m_streamEventsHandler, m_hostService );
    case RequestInfo::Format::Foscam:
        return std::make_shared< InputFCAudioStream >(
            std::move( audioStreamInfo ), m_streamEventsHandler, m_hostService );
    }
    return nullptr;
}

void AudioStreamManager::streamsInterruptedNotify() {
    const auto streams = m_dbClient.findAll( UiExtensionsCamerasDBName, JSON_EMPTY_OBJ, nullptr );
    for( const auto& stream : streams ) {
        Json userData;
        auto streamId = json_get_str( stream, db::ID_FIELD_NAME );
        json_get( stream, "userData", userData );

        m_streamEventsHandler.onStreamError( streamId, userData, "Stream was interrupted" );
    }
}

}  // namespace Cameras
}  // namespace HubPlatform
