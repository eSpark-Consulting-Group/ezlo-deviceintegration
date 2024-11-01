#include "uid/camera_addon_handler.h"
#include "json_utils/json_utils.h"
#include "jsonrpc/jsonrpc.h"
#include "base_utils/utils.h"
#include "system_utils/filesystem.h"
#include "common_utils/cpp14.h"
#include "http_bridge/session.h"
#include "http_bridge/ws_api_bridge.h"
#include "config_reader/config_keys.h"
#include "hub_settings/hub_settings_reader.h"
#include "crypto_utils/digests.h"
#include "system_utils/environment.h"

#include <dirent.h>
#include <sys/types.h>

namespace HubPlatform {
namespace Uid {

using namespace HttpBridge;
const int CamerasEndpointHandler::CameraAddonRequestTimeout;
static constexpr int DefaultTimeoutSec = 20;
static constexpr char const CamerasSnapshotIntervalSetting[] = "cameras.snapshot_cache_interval";

CamerasEndpointHandler::CamerasEndpointHandler( Services::Service& hostService )
: m_hostService( hostService ) {
    ConfigReader configReader;
    int timeoutValue = DefaultTimeoutSec;
    setConfigIntOption( configReader, ConfigKeys::CamerasSnapshotCacheTimeout, timeoutValue, DefaultTimeoutSec );
    m_defaultCacheTimeout = std::chrono::seconds( timeoutValue );
    m_requestsCache.clear();
    m_cachedImages.clear();

    m_cachedSnapshotsProcessing = hostService.createPostponedTask( [this]( Services::PostponedTask& ) {
        tryToClearNotProcessedImages(
            [this]( const std::string& streamId ) { removeCachedImageIfExpired( streamId ); } );

        requestsProcessing(
            [this]( HttpBridge::CallbackParams params, const std::string& streamId, const std::string& requestId ) {
                getSnapshotInternal( params, streamId, true, requestId );
            } );
        {
            std::lock_guard< std::mutex > lock( m_statusChangeMutex );
            if( m_cachedImages.empty() && m_requestsCache.empty() ) {
                m_cachedSnapshotsProcessing->stop();
            }
        }
    } );
}

std::chrono::seconds CamerasEndpointHandler::getTimeout() {
    auto timeout = m_defaultCacheTimeout;
    Hub::SettingsReader reader( m_hostService.getDBClient() );
    if( auto snapshotInterval = reader.findSetting( CamerasSnapshotIntervalSetting ) ) {
        timeout = std::chrono::seconds( snapshotInterval->getIntValue() );
    }

    return timeout;
}

void CamerasEndpointHandler::setConfigIntOption( ConfigReader& configReader,
                                                 const char* optionPath,
                                                 int& setValue,
                                                 int defaultValue ) {
    try {
        setValue = configReader.getInt( optionPath );
    }
    catch( std::exception& ex ) {
        setValue = defaultValue;
        Log::error( cc::error( "Unable found config option " ) + cc::note( optionPath ) + cc::error( ": " ) +
                    cc::attention( ex.what() ) );
        Log::error( cc::error( "Default value is set: " ) + cc::num( defaultValue ) );
    }
}

bool CamerasEndpointHandler::isProcessingStreamIdInRequestsCache( const std::string& streamId ) {
    std::lock_guard< std::mutex > lock( m_statusChangeMutex );
    for( auto& cachedReq : m_requestsCache ) {
        auto status = cachedReq.second.status;
        if( cachedReq.second.streamId == streamId && status == RequestStatus::PROCESSING ) {
            return true;
        }
    }
    return false;
}

void CamerasEndpointHandler::removeFile( const std::string& filePath ) const {
    if( !filesystem::removeFile( filePath ) ) {
        LOGGER_DEBUG( "File: " + filePath + " wasn't removed, try again" );
        if( !filesystem::removeFile( filePath ) ) {
            LOGGER_ERROR( "File: " + filePath + " can't removed" );
        }
    }
}

void CamerasEndpointHandler::removeUnusedSnapshots( const std::string& streamId ) {
    auto savedImagesNames = filesystem::getDirectoryFileNames( Environment::getSnapshotsFolder().c_str() );
    if( savedImagesNames.empty() )
        return;

    auto cachedImagePath = m_cachedImages[ streamId ].filePath;
    for( auto savedImagesFileName : savedImagesNames ) {
        if( savedImagesFileName.find( streamId ) != std::string::npos &&
            cachedImagePath.find( savedImagesFileName ) == std::string::npos ) {
            auto deletedImagePath = Environment::getSnapshotsFolder() + "/" + savedImagesFileName;
            LOGGER_DEBUG( "Remove unused snapshot image: " + deletedImagePath );
            removeFile( deletedImagePath );
        }
    }
}

void CamerasEndpointHandler::tryToClearNotProcessedImages( const RemoveUnusedSnapshotCallback& clearCallback ) {
    std::set< std::string > streamIds;
    for( auto& request : m_cachedImages ) {
        auto streamId = request.first;
        streamIds.insert( streamId );
    }

    for( auto& streamId : streamIds ) {
        removeImageInfoIfFileNotExist( streamId );
    }

    for( auto& request : m_cachedImages ) {
        auto streamId = request.first;
        streamIds.insert( streamId );
    }

    for( auto& streamId : streamIds ) {
        if( clearCallback && !isProcessingStreamIdInRequestsCache( streamId ) ) {
            clearCallback( streamId );
            break;
        }
    }
}

std::string CamerasEndpointHandler::getCachedImagePath( const std::string& streamId ) {
    auto imageInfo = m_cachedImages[ streamId ];
    auto filePath = imageInfo.filePath;

    return filePath;
}

std::string CamerasEndpointHandler::getCachedImageRequestId( const std::string& streamId ) {
    auto imageInfo = m_cachedImages[ streamId ];
    return imageInfo.requestId;
}

void CamerasEndpointHandler::removeCachedInfo( const std::string& streamId ) {
    m_cachedImages.erase( streamId );
}

bool CamerasEndpointHandler::isExpiredCachedImage( const std::string& streamId ) {
    auto now =
        std::chrono::duration_cast< std::chrono::seconds >( std::chrono::steady_clock::now().time_since_epoch() );
    auto imageInfo = m_cachedImages[ streamId ];
    auto timestamp = imageInfo.timestamp;
    auto timeout = now - timestamp;
    bool isExpired = timeout > imageInfo.timeout;
    return isExpired;
}

void CamerasEndpointHandler::removeCachedImageIfExpired( const std::string& streamId ) {
    if( isCachedImage( streamId ) ) {
        if( isExpiredCachedImage( streamId ) ) {
            auto imagePath = getCachedImagePath( streamId );
            auto requestId = getCachedImageRequestId( streamId );
            LOGGER_DEBUG( "To remove: imagePath: " + imagePath + ", requestId: " + requestId );
            removeCachedInfo( streamId );
            removeFile( imagePath.c_str() );
        }
    }
}

void CamerasEndpointHandler::appendRequest( HttpBridge::CallbackParams params,
                                            const std::string& streamId,
                                            const std::string& requestId ) {
    LOGGER_DEBUG( "streamId: " + streamId + ", requestId: " + requestId );
    {
        std::lock_guard< std::mutex > lock( m_statusChangeMutex );
        auto user = params.m_user;
        auto& requestInfo = m_requestsCache[ static_cast< UserData* >( user ) ];
        requestInfo.streamId = streamId;
        requestInfo.status = RequestStatus::APPEND;
        requestInfo.wsi = params.m_wsi;
        requestInfo.requestId = requestId;
    }

    if( !m_cachedSnapshotsProcessing->isActive() ) {
        m_cachedSnapshotsProcessing->start( std::chrono::milliseconds( 100 ), true );
    }
}

void CamerasEndpointHandler::requestsProcessing( const SnapshotProcessingCallback& makeStreamSnapshot ) {
    std::set< std::string > streamIds = getStreamIdsFromCache();
    for( auto& streamId : streamIds ) {
        requestProcessingByStreamId( makeStreamSnapshot, streamId );
    }
}

void CamerasEndpointHandler::requestProcessingByStreamId( const SnapshotProcessingCallback& makeStreamSnapshot,
                                                          const std::basic_string< char >& streamId ) {
    std::unique_lock< std::mutex > lock( m_statusChangeMutex );
    for( auto cachedReqIt = m_requestsCache.begin(); cachedReqIt != m_requestsCache.end(); ) {
        auto request = cachedReqIt->second;
        if( request.streamId == streamId ) {
            if( request.status == RequestStatus::DONE ) {
                m_requestsCache.erase( cachedReqIt++ );
                continue;
            }
            else if( request.status == RequestStatus::PROCESSING ) {
                return;
            }
            else {  // request.status == RequestStatus::APPEND
                lock.unlock();
                makeSnapshot( makeStreamSnapshot, cachedReqIt, request );
                return;
            }
        }
        else {
            return;
        }
    }
}

std::set< std::string > CamerasEndpointHandler::getStreamIdsFromCache() {
    std::set< std::string > streamIds;
    std::lock_guard< std::mutex > lock( m_statusChangeMutex );
    for( auto& request : m_requestsCache ) {
        auto streamId = request.second.streamId;
        streamIds.insert( streamId );
    }
    return streamIds;
}

void CamerasEndpointHandler::makeSnapshot( const SnapshotProcessingCallback& makeStreamSnapshot,
                                           const std::map< UserData*, RequestInfo >::iterator& cachedReqIt,
                                           const RequestInfo& request ) {
    if( makeStreamSnapshot ) {
        CallbackParams params( request.wsi, (lws_callback_reasons) 0, cachedReqIt->first, nullptr, 0 );
        setRequestStatus( params, RequestStatus::PROCESSING );
        makeStreamSnapshot( params, request.streamId, request.requestId );
    }
}

void CamerasEndpointHandler::setRequestStatus( HttpBridge::CallbackParams params, RequestStatus status ) {
    std::lock_guard< std::mutex > lock( m_statusChangeMutex );
    if( m_requestsCache.find( static_cast< UserData* >( params.m_user ) ) != m_requestsCache.end() ) {
        auto& request = m_requestsCache[ static_cast< UserData* >( params.m_user ) ];
        request.status = status;
        auto requestId = request.requestId;
        LOGGER_DEBUG( "requestId: " + requestId + ", status: " + cc::num10( (int) status ) );
    }
}

bool CamerasEndpointHandler::isCachedImage( const std::string& streamId ) {
    auto imageInfo = m_cachedImages.find( streamId );
    if( imageInfo == m_cachedImages.end() ) {
        return false;
    }
    auto imagePath = imageInfo->second.filePath;
    bool isExist = filesystem::isFileExist( imagePath );
    return isExist;
}

void CamerasEndpointHandler::removeImageInfoIfFileNotExist( const std::string& streamId ) {
    auto imageInfo = m_cachedImages.find( streamId );
    if( imageInfo == m_cachedImages.end() ) {
        return;
    }
    auto imagePath = imageInfo->second.filePath;
    bool isExist = filesystem::isFileExist( imagePath );
    if( !isExist ) {
        removeCachedInfo( streamId );
    }
}

void CamerasEndpointHandler::sendFile( const std::string& streamId, HttpBridge::CallbackParams params ) {
    lws_callback_on_writable( params.m_wsi );
}

std::string CamerasEndpointHandler::getCachedFilePath( const std::string& streamId ) {
    return m_cachedImages[ streamId ].filePath;
}

void CamerasEndpointHandler::addFilePathToCache( const std::string& streamId,
                                                 const std::string& filePath,
                                                 const std::string& requestId ) {
    if( !filePath.empty() ) {
        if( m_cachedImages.find( streamId ) == m_cachedImages.end() ) {
            m_cachedImages[ streamId ].filePath = filePath;

            auto now = std::chrono::duration_cast< std::chrono::seconds >(
                std::chrono::steady_clock::now().time_since_epoch() );
            m_cachedImages[ streamId ].timestamp = now;
            m_cachedImages[ streamId ].realTime = std::chrono::system_clock::now();
            m_cachedImages[ streamId ].timeout = getTimeout();
            m_cachedImages[ streamId ].requestId = requestId;
        }
    }
    else {
        LOGGER_ERROR( "Snapshot wasn't created" );
    }
}

void CamerasEndpointHandler::getSnapshot( HttpBridge::CallbackParams params,
                                          const std::string& streamId,
                                          bool cached,
                                          const std::string& requestId ) {
    if( cached && getTimeout() > std::chrono::seconds( 0 ) ) {
        appendRequest( params, streamId, requestId );
    }
    else {
        getSnapshotInternal( params, streamId, false, requestId );
    }
}
void CamerasEndpointHandler::getSnapshotInternal( HttpBridge::CallbackParams params,
                                                  const std::string& streamId,
                                                  bool cached,
                                                  const std::string& requestId ) {
    LOGGER_DEBUG( "cached: " + cc::flag( cached ) + ", streamId: " + streamId + ", requestId: " + requestId );
    auto callback = [this, params, streamId, cached, requestId]( const Json& message ) {
        auto filePath = json_get_str( message, "filePath" );
        LOGGER_DEBUG_RAW( "Get image for streamId: " + streamId + ", requestId: " + requestId );
        utils::AfterReturn afterReturn( [this, params, filePath, streamId, cached, requestId]() {
            if( isSessionClosed( params.m_wsi ) ) {
                if( !filePath.empty() ) {
                    Log::trace( "remove snapshot for stream " + cc::attention( streamId ) );
                    removeFile( filePath.c_str() );
                }
                return;
            }
            if( cached ) {
                imageIsCompleted( params );
            }
            LOGGER_DEBUG_RAW( "Send snapshot " + streamId + ", requestId: " + requestId );
            callbackOnWritable( params );
            LOGGER_DEBUG_RAW( "Send snapshot - done streamId: " + streamId + ", requestId: " + requestId );
        } );
        std::lock_guard< std::mutex > lock( m_mutex );
        Log::trace( "response from camera addon for stream " + cc::attention( streamId ) + ": " + cc::j( message ) );
        if( hasError( message ) ) {
            std::string error = json_get_str( message[ "error" ], "message" );
            Log::error( cc::error( error + ": " ) + cc::attention( streamId ) );
            m_files[ params.m_user ].error = error;
            return;
        }
        if( filePath.empty() || !filesystem::getFileSize( filePath ) ) {
            const std::string& error = "empty file from camera addon";
            Log::error( cc::error( error + ": " ) + cc::attention( streamId ) );
            m_files[ params.m_user ].error = error;
            return;
        }
        auto file = File( fopen( filePath.c_str(), "rb" ), []( FILE* file ) {
            if( file )
                fclose( file );
        } );
        if( file == nullptr ) {
            const std::string& error = "can not open file";
            Log::error( cc::error( error + ": " ) + cc::attention( streamId ) );
            m_files[ params.m_user ].error = error;
            return;
        }

        auto checksum = logFileChecksum( filePath );
        LOGGER_DEBUG_RAW( "Image checksum: " + checksum + ", streamId: " + streamId + ", requestId: " + requestId +
                          ", filename: " + filePath );

        auto& state = m_files[ params.m_user ];
        state.file = std::move( file );
        state.error = "";
        state.fileSize = filesystem::getFileSize( filePath.c_str() );
        state.fileAutoRemover = FileAutoRemover{ filePath, cached };
        state.streamId = streamId;
        state.filePath = filePath;

        if( cached ) {
            addFilePathToCache( streamId, filePath, requestId );
            removeUnusedSnapshots( streamId );
        }
    };
    auto timeoutCallback = [this, params, streamId, requestId]() {
        LOGGER_DEBUG_RAW( "timeout when get image streamId: " + streamId + ", requestId: " + requestId );
        if( isSessionClosed( params.m_wsi ) ) {
            return;
        }
        const std::string& error = "no response from camera addon";
        Log::error( cc::error( error + ": " ) + cc::attention( streamId ) );
        std::lock_guard< std::mutex > lock( m_mutex );
        m_files[ params.m_user ].error = error;
        callbackOnWritable( params );
        LOGGER_DEBUG_RAW( "Send timeout  - done streamId: " + streamId + ", requestId: " + requestId );
    };

    if( cached ) {
        removeCachedImageIfExpired( streamId );
    }
    if( cached && isCachedImage( streamId ) ) {
        LOGGER_DEBUG_RAW( "Get cached image for streamId: " + streamId + ", requestId: " + requestId );
        std::string filePath = getCachedFilePath( streamId );
        auto checksum = logFileChecksum( filePath );
        LOGGER_DEBUG_RAW( "Cached image checksum: " + checksum + ", streamId: " + streamId +
                          ", requestId: " + requestId + ", filename: " + filePath );
        auto file = File( fopen( filePath.c_str(), "rb" ), []( FILE* file ) {
            if( file ) {
                fclose( file );
            }
        } );
        LOGGER_DEBUG_RAW( "Prepare cached image params: " + streamId + " requestId: " + requestId );
        std::lock_guard< std::mutex > lock( m_mutex );
        auto& state = m_files[ params.m_user ];
        state.file = std::move( file );
        state.error = "";
        state.fileSize = filesystem::getFileSize( filePath.c_str() );
        state.fileAutoRemover = FileAutoRemover{ filePath, cached };
        state.streamId = streamId;
        state.requestId = requestId;
        LOGGER_DEBUG_RAW( "Mark cached image as completed, streamId: " + streamId + ", requestId: " + requestId );
        imageIsCompleted( params );
        LOGGER_DEBUG_RAW( "Send cached snapshot, streamId: " + streamId + ", requestId: " + requestId );
        sendFile( streamId, params );
        LOGGER_DEBUG_RAW( "Send cached snapshot  - done streamId: " + streamId + ", requestId: " + requestId );
    }
    else {
        LOGGER_DEBUG_RAW( "Send async request to make snapshot  streamId: " + streamId + " requestId: " + requestId );
        m_hostService.sendAsyncRequest( "addon.cameras",
                                        JSON( "streamId" << streamId << "requestId" << requestId ),
                                        CAMERAS_MAKE_SNAPSHOT_FOR_UI,
                                        callback,
                                        timeoutCallback,
                                        std::chrono::seconds( CameraAddonRequestTimeout ) );
        LOGGER_DEBUG_RAW( "Send async request - done " + streamId + " requestId: " + requestId + " - done" );
    }
}

std::string CamerasEndpointHandler::logFileChecksum( const std::string& filePath ) const {
    std::string result;
    std::ifstream file( filePath.c_str(), std::ios::binary );
    if( !file ) {
        LOGGER_ERROR_RAW( cc::error( "File not found: " + filePath ) );
        return "";
    }
    else {
        result = CryptoUtils::Digests::md5( file );
    }
    return result;
}

bool CamerasEndpointHandler::isHandlerValid() {
    return true;
}

bool CamerasEndpointHandler::isSessionClosed( lws* wsi ) const {
    HttpBridge::SessionPtr pss = HttpBridge::getApiBridge().findSession( wsi );
    if( !pss || !pss->getData() ) {
        Log::error( cc::error( "Http session is not found for " ) + cc::num0x( reinterpret_cast< uint64_t >( wsi ) ) );
        return true;
    }
    auto& sessionData = *pss->getData();

    if( sessionData.isFinished() ) {
        Log::warn( cc::warn( "Http session is closed" ) );
        return true;
    }
    return false;
}

void CamerasEndpointHandler::imageIsCompleted( HttpBridge::CallbackParams params ) {
    setRequestStatus( params, RequestStatus::DONE );
}

time_t CamerasEndpointHandler::getCachedTimestamp( HttpBridge::CallbackParams params ) {
    std::lock_guard< std::mutex > lock( m_statusChangeMutex );
    auto request = m_requestsCache.find( static_cast< UserData* >( params.m_user ) );
    if( request != m_requestsCache.end() ) {
        auto streamId = request->second.streamId;
        auto timePoint = m_cachedImages[ streamId ].realTime;
        auto time = timePoint.time_since_epoch().count();
        if( time > 0 ) {
            return std::chrono::system_clock::to_time_t( timePoint );
        }
    }
    return 0;
}

time_t CamerasEndpointHandler::getCachedEndTimestamp( HttpBridge::CallbackParams params ) {
    std::lock_guard< std::mutex > lock( m_statusChangeMutex );
    auto request = m_requestsCache.find( static_cast< UserData* >( params.m_user ) );
    if( request != m_requestsCache.end() ) {
        auto streamId = request->second.streamId;
        auto timePoint = m_cachedImages[ streamId ].realTime;
        auto timeout = m_cachedImages[ streamId ].timeout;
        auto time = timePoint.time_since_epoch().count();
        if( time > 0 ) {
            auto newTimePoint = timePoint + timeout;
            return std::chrono::system_clock::to_time_t( newTimePoint );
        }
    }

    return 0;
}

}  // namespace Uid
}  // namespace HubPlatform
