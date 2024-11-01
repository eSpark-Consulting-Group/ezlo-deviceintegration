#include "snapshot_cache.h"
#include "log_utils/ezlo_log.h"
#include "json_utils/json_utils.h"
#include "database_client/dbqueryparser.h"
#include "database_client/idbclient.h"
#include "database_client/dbid.h"
#include <chrono>

namespace HubPlatform {
namespace Cameras {

SnapshotCache::SnapshotCache( IDbClient& dbClient, int cacheTimeoutSec )
: m_dbClient( dbClient )
, m_cacheTimeout( cacheTimeoutSec ) {
    dbClient.clear( TempCameraSnapshotCacheDBName );
    dbClient.clear( TempCameraSnapshotRequestsQueueDBName );
}

IDbClient& SnapshotCache::getDBClient() {
    return m_dbClient;
}

// Add snapshot to cache
void SnapshotCache::addSnapshotToCache( const std::string& streamId, const Json& imageInfo, const Json& metadata ) {
    auto filePath = json_get_str( imageInfo, "filePath" );
    auto& dbClient = getDBClient();
    if( !isCachedImage( streamId ) ) {
        auto timestamp =
            std::chrono::duration_cast< std::chrono::seconds >( std::chrono::steady_clock::now().time_since_epoch() )
                .count();
        auto query = JSON( "_id" << streamId << "filePath" << filePath << "timestamp" << timestamp << "metadata"
                                 << metadata << "imageInfo" << imageInfo );
        dbClient.insert( TempCameraSnapshotCacheDBName, query, true );
    }
}

bool SnapshotCache::isCachedImage( const std::string& streamId ) {
    auto info = m_dbClient.findOne( TempCameraSnapshotCacheDBName, JSON( "_id" << streamId ), nullptr );
    return !info.empty();
}

bool SnapshotCache::isExpiredCachedImage( const std::string& streamId ) {
    auto now = std::chrono::duration_cast< std::chrono::seconds >( std::chrono::steady_clock::now().time_since_epoch() )
                   .count();
    auto imageInfo = m_dbClient.findOne( TempCameraSnapshotCacheDBName, JSON( "_id" << streamId ), nullptr );
    auto timestamp = json_get_int( imageInfo, "timestamp" );
    auto timeout = now - timestamp;
    bool isExpired = timeout > m_cacheTimeout.count();

    return isExpired;
}

std::string SnapshotCache::getCachedImagePath( const std::string& streamId ) {
    auto imageInfo = m_dbClient.findOne( TempCameraSnapshotCacheDBName, JSON( "_id" << streamId ), nullptr );
    auto filePath = json_get_str( imageInfo, "filePath" );
    return filePath;
}

Json SnapshotCache::getCachedImageInfo( const std::string& streamId ) {
    auto record = m_dbClient.findOne( TempCameraSnapshotCacheDBName, JSON( "_id" << streamId ), nullptr );
    Json imageInfo;
    json_get( record, "imageInfo", imageInfo );
    return imageInfo;
}

void SnapshotCache::removeCachedInfo( const std::string& streamId ) {
    auto imageInfo = m_dbClient.findOne( TempCameraSnapshotCacheDBName, JSON( "_id" << streamId ), nullptr );
    m_dbClient.remove( TempCameraSnapshotCacheDBName, JSON( "_id" << streamId ) );
}

Json SnapshotCache::getCachedImageMetadata( const std::string& streamId ) {
    auto imageInfo = m_dbClient.findOne( TempCameraSnapshotCacheDBName, JSON( "_id" << streamId ), nullptr );
    Json metadata;
    json_get( imageInfo, "metadata", metadata );

    return metadata;
}

std::string SnapshotCache::appendRequest( const JsonObject& params ) {
    auto record = params;
    auto snapshotId = json_get_str( record, "snapshotId" );
    record[ "status" ] = APPEND;
    auto id = db::DbId::genOID();
    record[ "_id" ] = id;
    getDBClient().insert( TempCameraSnapshotRequestsQueueDBName, record );
    return id;
}

void SnapshotCache::setRequestStatusDone( const std::string& cacheId ) {
    setRequestStatus( cacheId, DONE );
}

void SnapshotCache::setRequestStatusProcessing( const std::string& cacheId ) {
    setRequestStatus( cacheId, PROCESSING );
}

void SnapshotCache::setRequestStatus( const std::string& cacheId, RequestStatus status ) {
    auto updated = getDBClient().findOne( TempCameraSnapshotRequestsQueueDBName, JSON( "_id" << cacheId ), nullptr );
    updated[ "status" ] = status;
    getDBClient().update( TempCameraSnapshotRequestsQueueDBName, JSON( "_id" << cacheId ), updated );
}

void SnapshotCache::requestsProcessing( const SnapshotProcessingCallback& makeStreamSnapshotForCloud ) {
    Json fieldsToReturn = JSON( "streamId" << 1 );
    auto recordsStreamIds =
        getDBClient().findAll( TempCameraSnapshotRequestsQueueDBName, JSON_EMPTY_OBJ, &fieldsToReturn );

    std::set< std::string > streamIds;

    for( auto& record : recordsStreamIds ) {
        streamIds.insert( json_get_str( record, "streamId" ) );
    }
    for( auto& streamId : streamIds ) {
        auto records =
            getDBClient().findAll( TempCameraSnapshotRequestsQueueDBName, JSON( "streamId" << streamId ), nullptr );
        for( auto& record : records ) {
            auto id = json_get_str( record, "_id" );
            auto status = json_get_int( record, "status" );
            auto snapshotId = json_get_str( record, "snapshotId" );
            Json metadata;
            json_get( record, "metadata", metadata );
            if( status == RequestStatus::PROCESSING )
                break;
            // if snapshot sent - remove it from cache
            if( status == RequestStatus::DONE ) {
                getDBClient().remove( TempCameraSnapshotRequestsQueueDBName, JSON( "id" << id ) );
            }
            else {
                auto params = JSON( "streamId" << streamId << "snapshotId" << snapshotId << "metadata" << metadata
                                               << "cacheId" << id );
                setRequestStatusProcessing( id );
                if( makeStreamSnapshotForCloud ) {
                    makeStreamSnapshotForCloud( params );
                }
                break;
            }
        }
    }
}

bool SnapshotCache::isProcessingStreamIdInRequestsCache( const std::string& streamId ) {
    Json fieldsToReturn = JSON( "status" << 1 );
    auto records =
        getDBClient().findAll( TempCameraSnapshotRequestsQueueDBName, JSON( streamId << streamId ), &fieldsToReturn );
    for( auto& record : records ) {
        auto status = json_get_int( record, "status" );
        if( status == RequestStatus::PROCESSING ) {
            return true;
        }
    }
    return false;
}

void SnapshotCache::tryToClearNotProcessedImages( const RemoveUnusedSnapshotCallback& clearCallback ) {
    Json fieldsToReturn = JSON( "_id" << 1 );
    auto records = m_dbClient.findAll( TempCameraSnapshotCacheDBName, JSON_EMPTY_OBJ, nullptr );
    for( auto& record : records ) {
        auto streamId = json_get_str( record, "_id" );
        if( clearCallback && !isProcessingStreamIdInRequestsCache( streamId ) ) {
            clearCallback( streamId );
        }
    }
}

}  // namespace Cameras
}  // namespace HubPlatform
