
#include "hardware_manager/speaker/playing_info_storage.h"

#include "hardware_manager/speaker/model/speaker_errors.h"
#include "hardware_manager/speaker/model/resources.h"
#include "hardware_manager/speaker/imessanger.h"

#include "tiny/tiny_consolecolors.h"
#include "log_utils/ezlo_log.h"
#include "system_utils/system.h"

#include "database_client/db_tables_names.h"
#include "database_client/idbclient.h"
#include "database_client/dbqueryparser.h"

#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace HardwareManager {
namespace Speaker {

PlayingInfoStorage::PlayingInfoStorage( IMessenger& luaMessenger, IDbClient& database )
: m_luaMessenger( luaMessenger )
, m_database( database ) {}

void PlayingInfoStorage::processAllPlayingInfo( OnPlayingInfo callback ) {
    const JsonObject fieldsToReturn = JSON( Fields::id << Fields::id );
    const auto allProcessInfoJson = m_database.findAll( TempPlayingInfoDBName, JSON_EMPTY_OBJ, &fieldsToReturn );

    PlayingInfo playingInfo;
    for( const auto& object : allProcessInfoJson ) {
        playingInfo = getInfo( json_get_str( object, Fields::id ) );
        if( !playingInfo.getId().empty() ) {
            callback( playingInfo );
        }
        else {
            LOGGER_WARN( cc::warn( "Cannot get playing info for id: " ) + cc::j( object ) );
        }
    }
}

bool PlayingInfoStorage::hasInfo( const std::string& soundId ) {
    return m_database.has( TempPlayingInfoDBName, JSON( Fields::id << soundId ) );
}

PlayingInfo PlayingInfoStorage::getInfo( const std::string& soundId ) {
    auto playingInfo = m_database.findOne( TempPlayingInfoDBName, JSON( Fields::id << soundId ), nullptr );
    if( !playingInfo.empty() ) {
        PlayingInfo resultInfo;
        if( resultInfo.fromJson( playingInfo ) ) {
            return resultInfo;
        }
    }

    return {};
}

void PlayingInfoStorage::addInfo( const PlayingInfo& info ) {
    m_database.insert( TempPlayingInfoDBName, info.toJson() );
}

void PlayingInfoStorage::removeInfo( const std::string& soundId ) {
    m_database.remove( TempPlayingInfoDBName, JSON( Fields::id << soundId ) );
}

void PlayingInfoStorage::setInfoStatus( const std::string& commandId, PlayingStatus value, ErrorInfo::Code errorCode ) {
    const auto playingInfo = getInfo( commandId );
    if( !playingInfo.getId().empty() ) {
        if( playingInfo.getStatus() != value ) {
            m_database.update( TempPlayingInfoDBName,
                               JSON( Fields::id << commandId ),
                               JSON( "$set" << JSON( Fields::status << toString( value ) ) ),
                               false,
                               true );

            m_luaMessenger.sendEvent( playingInfo, value, errorCode );
        }
    }
    else {
        LOGGER_WARN( cc::warn( "Cannot get playing info for id: " ) + cc::note( commandId ) +
                     cc::warn( ". New status has not been applied." ) );
    }
}

void PlayingInfoStorage::saveProcessInfo( const PlayingProcessInfo& processInfo ) {
    m_database.insert( TempPlayingInfoStartedDBName, processInfo.toJson() );
}

PlayingProcessInfo PlayingInfoStorage::getProcessInfo( const std::string& soundId ) {
    auto processInfoJson = m_database.findOne( TempPlayingInfoStartedDBName, JSON( Fields::id << soundId ), nullptr );
    if( !processInfoJson.empty() ) {
        PlayingProcessInfo resultInfo;
        if( resultInfo.fromJson( processInfoJson ) ) {
            return resultInfo;
        }
    }

    return {};
}

void PlayingInfoStorage::removeProcessInfo( const std::string& soundId ) {
    m_database.remove( TempPlayingInfoStartedDBName, JSON( Fields::id << soundId ) );
}

void PlayingInfoStorage::processAllStartedSoundsInfo( OnPlayingProcessInfo callback ) {
    const JsonObject fieldsToReturn = JSON( Fields::id << Fields::id );
    const auto allProcessInfoJson = m_database.findAll( TempPlayingInfoStartedDBName, JSON_EMPTY_OBJ, &fieldsToReturn );

    PlayingProcessInfo processInfo;
    for( const auto& object : allProcessInfoJson ) {
        processInfo = getProcessInfo( json_get_str( object, Fields::id ) );
        if( !processInfo.getId().empty() ) {
            callback( processInfo );
        }
        else {
            LOGGER_WARN( cc::warn( "Cannot get process info for id: " ) + cc::j( object ) );
        }
    }
}

}  // namespace Speaker
}  // namespace HardwareManager
}  // namespace HubPlatform
