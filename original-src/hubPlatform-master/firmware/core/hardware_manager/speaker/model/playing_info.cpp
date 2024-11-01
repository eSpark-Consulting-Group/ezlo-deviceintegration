
#include "hardware_manager/speaker/model/playing_info.h"
#include "hardware_manager/speaker/model/resources.h"

#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace HardwareManager {
namespace Speaker {

PlayingInfo::PlayingInfo( std::string id,
                          std::string songFilePath,
                          std::int64_t time,
                          std::uint8_t volume,
                          std::string plugin,
                          std::string handlerScript,
                          Json userData )
: m_id( std::move( id ) )
, m_songFilePath( std::move( songFilePath ) )
, m_time( time )
, m_volume( volume )
, m_plugin( std::move( plugin ) )
, m_handlerScript( std::move( handlerScript ) )
, m_userData( std::move( userData ) ) {}

const std::string& PlayingInfo::getId() const {
    return m_id;
}

const std::string& PlayingInfo::getSongFilePath() const {
    return m_songFilePath;
}

std::int64_t PlayingInfo::getPlayingTime() const {
    return m_time;
}

std::uint8_t PlayingInfo::getVolume() const {
    return m_volume;
}

PlayingStatus PlayingInfo::getStatus() const {
    return m_status;
}

const std::string& PlayingInfo::getPluginName() const {
    return m_plugin;
}

const std::string& PlayingInfo::getHandlerScriptPath() const {
    return m_handlerScript;
}

const Json& PlayingInfo::getUserData() const {
    return m_userData;
}

void PlayingInfo::setPlayingTime( std::int64_t playingTime ) {
    m_time = playingTime;
}

Json PlayingInfo::toJson() const {
    Json result = { { Fields::id, m_id },     { Fields::plugin, m_plugin }, { Fields::path, m_songFilePath },
                    { Fields::time, m_time }, { Fields::volume, m_volume }, { Fields::status, toString( m_status ) } };

    if( !m_handlerScript.empty() ) {
        result[ Fields::handler ] = m_handlerScript;
    }

    if( !m_userData.empty() ) {
        result[ Fields::userData ] = m_userData;
    }

    return result;
}

bool PlayingInfo::fromJson( const Json& data ) {
    std::string status;
    if( ( !json_get( data, Fields::status, status ) ) || status.empty() ) {
        return false;
    }

    m_status = toPlayingStatus( status.c_str() );
    json_get( data, Fields::handler, m_handlerScript );

    const auto itFind = data.find( Fields::userData );
    if( itFind != data.end() ) {
        m_userData = *itFind;
    }

    return json_get( data, Fields::id, m_id ) && json_get( data, Fields::plugin, m_plugin ) &&
           json_get( data, Fields::path, m_songFilePath ) && json_get( data, Fields::time, m_time ) &&
           json_get( data, Fields::volume, m_volume );
}

}  // namespace Speaker
}  // namespace HardwareManager
}  // namespace HubPlatform