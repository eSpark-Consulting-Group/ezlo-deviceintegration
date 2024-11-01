#pragma once
#include "http_bridge/cameras_addon_endpoint.h"
#include "service_interfaces/service.h"
#include "config_reader/config_reader.h"
#include <mutex>
#include <chrono>

namespace HubPlatform {
namespace Uid {

struct UserData {};

enum class RequestStatus : int { APPEND = 1, PROCESSING = 2, DONE = 3 };
using SnapshotProcessingCallback = std::function<
    void( HttpBridge::CallbackParams params, const std::string& streamId, const std::string& requestId ) >;
using RemoveUnusedSnapshotCallback = std::function< void( const std::string& ) >;
struct ImageInfo {
    std::string filePath;
    std::chrono::seconds timestamp;
    std::chrono::system_clock::time_point realTime;
    std::chrono::seconds timeout;
    std::string requestId;
};

struct RequestInfo {
    struct lws* wsi;
    RequestStatus status;
    std::string streamId;
    std::string requestId;
};

class CamerasEndpointHandler : public HttpBridge::ICamerasEndpointHandler {
public:
    CamerasEndpointHandler( Services::Service& hostService );
    virtual void getSnapshot( HttpBridge::CallbackParams params,
                              const std::string& streamId,
                              bool cached,
                              const std::string& requestId ) override;
    virtual bool isHandlerValid() override;
    virtual time_t getCachedEndTimestamp( HttpBridge::CallbackParams params ) override;
    virtual time_t getCachedTimestamp( HttpBridge::CallbackParams params ) override;

private:
    bool isSessionClosed( struct lws* wsi ) const;
    void appendRequest( HttpBridge::CallbackParams params, const std::string& streamId, const std::string& requestId );
    bool isCachedImage( const std::string& streamId );
    void requestsProcessing( const SnapshotProcessingCallback& makeStreamSnapshotForCloud );
    void tryToClearNotProcessedImages( const RemoveUnusedSnapshotCallback& clearCallback );
    bool isProcessingStreamIdInRequestsCache( const std::string& streamId );
    bool isExpiredCachedImage( const std::string& streamId );
    void removeCachedImageIfExpired( const std::string& streamId );
    void getSnapshotInternal( HttpBridge::CallbackParams params,
                              const std::string& streamId,
                              bool cached,
                              const std::string& requestId );
    void sendFile( const std::string& streamId, HttpBridge::CallbackParams params );
    std::string getCachedFilePath( const std::string& streamId );
    void addFilePathToCache( const std::string& streamId, const std::string& filePath, const std::string& requestId );
    std::string getCachedImagePath( const std::string& streamId );
    std::string getCachedImageRequestId( const std::string& streamId );
    void removeCachedInfo( const std::string& streamId );
    void setRequestStatus( HttpBridge::CallbackParams params, RequestStatus status );
    void makeSnapshot( const SnapshotProcessingCallback& makeStreamSnapshot,
                       const std::map< UserData*, RequestInfo >::iterator& cachedReqIt,
                       const RequestInfo& request );
    std::set< std::string > getStreamIdsFromCache();
    void requestProcessingByStreamId( const SnapshotProcessingCallback& makeStreamSnapshot,
                                      const std::basic_string< char >& streamId );
    void setConfigIntOption( ConfigReader& configReader, const char* optionPath, int& setValue, int defaultValue );
    void imageIsCompleted( HttpBridge::CallbackParams params );
    void removeImageInfoIfFileNotExist( const std::string& streamId );
    std::chrono::seconds getTimeout();
    std::string logFileChecksum( const std::string& filePath ) const;
    void removeUnusedSnapshots( const std::string& streamId );
    void removeFile( const std::string& filePath ) const;

private:
    Services::Service& m_hostService;
    static const int CameraAddonRequestTimeout = 60;
    std::chrono::seconds m_defaultCacheTimeout{ 20 };
    Services::PostponedTaskPtr m_cachedSnapshotsProcessing;
    std::map< std::string, ImageInfo > m_cachedImages;
    mutable std::mutex m_statusChangeMutex;
    std::map< UserData*, RequestInfo > m_requestsCache;
};

}  // namespace Uid
}  // namespace HubPlatform
