#pragma once

#include "json_utils/json_fwd.h"

#include <chrono>
#include <string>

class IDbClient;

namespace HubPlatform {
namespace Cameras {

enum RequestStatus { APPEND = 1, PROCESSING = 2, DONE = 3 };

class SnapshotCache;
using SnapshotCachePtr = std::unique_ptr< SnapshotCache >;
using SnapshotProcessingCallback = std::function< void( const Json& ) >;
using RemoveUnusedSnapshotCallback = std::function< void( const std::string& ) >;

class SnapshotCache {
public:
    explicit SnapshotCache( IDbClient& dbClient, int cacheTimeoutSec );

    // snapshot processing
    void addSnapshotToCache( const std::string& streamId, const Json& imageInfo, const Json& metadata );
    bool isCachedImage( const std::string& streamId );
    std::string getCachedImagePath( const std::string& streamId );
    Json getCachedImageMetadata( const std::string& streamId );
    bool isExpiredCachedImage( const std::string& streamId );
    void removeCachedInfo( const std::string& streamId );
    Json getCachedImageInfo( const std::string& streamId );

    // request processing
    std::string appendRequest( const JsonObject& params );
    void setRequestStatusDone( const std::string& cacheId );
    void requestsProcessing( const SnapshotProcessingCallback& makeStreamSnapshotForCloud );
    void tryToClearNotProcessedImages( const RemoveUnusedSnapshotCallback& clearCallback );

private:
    IDbClient& getDBClient();
    void setRequestStatus( const std::string& cacheId, RequestStatus status );
    void setRequestStatusProcessing( const std::string& cacheId );
    bool isProcessingStreamIdInRequestsCache( const std::string& streamId );

private:
    IDbClient& m_dbClient;
    std::chrono::seconds m_cacheTimeout;
};

}  // namespace Cameras
}  // namespace HubPlatform
