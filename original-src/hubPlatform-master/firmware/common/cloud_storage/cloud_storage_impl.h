#pragma once

#include "cloud_storage/cloud_storage_type.h"
#include "cloud_storage/response_dispatcher.h"
#include "cloud_storage/controller_token.h"
#include "json_utils/json_fwd.h"
#include "http/http.h"

#include <string>
#include <chrono>

class IDbClient;

namespace HubPlatform {

namespace CommonHttp {
struct HttpOptions;
}  // namespace CommonHttp
namespace Common {
namespace UT {
class CloudStorageImplTests;
}
}  // namespace Common

namespace Common {
namespace CloudStorageInterface {

class CloudStorageImpl final {
    friend class HubPlatform::Common::UT::CloudStorageImplTests;

public:
    using UploadResponse = std::function< void( Json userData, const std::string& error, const FileKey& fileKey ) >;
    using DownloadResponse = std::function< void( Json userData, const std::string& error ) >;
    using Path = std::string;
    using RequestId = CommonHttp::Http::RequestId;
    using UploadResponses = std::map< RequestId, UploadResponse >;

public:
    explicit CloudStorageImpl( IDbClient& dbClient );
    ~CloudStorageImpl();

    /* @brief return file key as response. In case of error returns empty key */
    Errors uploadFile( const UploadFileData& data, StorageType storageType, UploadResponse response );
    Errors uploadBackup( const UploadFileData& data, StorageType storageType, UploadResponse response );
    Errors uploadLogs( const UploadFileData& data, StorageType storageType, UploadResponse response );
    Errors downloadFile( const FileKey& fileKey, const Path& downloadPath, DownloadResponse downloadCallback );
    Errors downloadFile( const ControllerToken& controllerToken,
                         LoadType loadTypeExact,
                         const FileKey& fileKey,
                         const Path& downloadPath,
                         DownloadResponse downloadCallback );

    void stopTransfer();

    void waitTillFinish();
    void exitDelayInterval( std::chrono::seconds delay );
    void replaceExternalUrlRule( const JsonObject& replacements );

private:
    const std::chrono::seconds m_requestTimeout = std::chrono::seconds( 90 );

private:
    Errors uploadFile( LoadType loadType,
                       const UploadFileData& data,
                       StorageType storageType,
                       StorageBucket storageBucket,
                       UploadResponse responseCallback );

    std::pair< Errors, CommonHttp::Http::RequestId > sendHttpRequest( CommonHttp::HttpOptions& httpOptions );
    void setHttpReceivingMethod();
    void setHttpDataSendMethod();
    void setHttpClosingConnectionMethod();

    void httpLegacyDataReceived( const std::string& /*eventHandler*/, const CommonHttp::EventPtr& event );
    void httpLegacyConnectionClosed( const std::string& /*eventHandler*/, const CommonHttp::EventPtr& event );

    void httpDataReceived( const std::string& /*eventHandler*/, const CommonHttp::EventPtr& event );
    void httpDataSend( const std::string& /* eventHandler */, const CommonHttp::EventPtr& event );
    void httpConnectionClosed( const std::string& /*eventHandler*/, const CommonHttp::EventPtr& event );

    static void sendResponse( const Json& message, const UploadResponse& callback, const std::string& errorMessage );

    bool onUploadRedirectStepOneCallback( HttpResponse response, const UploadResponse& responseCallback );
    bool onUploadRedirectFileBodyCallback( HttpResponse response, const UploadResponse& responseCallback );
    static bool onUploadRedirectFinishedCallback( HttpResponse response, const UploadResponse& responseCallback );
    void onDownloadFinishedCallback( const HttpResponse& response,
                                     const ControllerToken& controllerToken,
                                     const DownloadResponse& downloadCallback );
    static std::string makeHttpErrorMessage( const HttpResponse& response );
    Json& makeReplacements( Json& stepOneResponse ) const;

    static LoadType selectLoadType( LoadType baseLoadType, const Endpoints& endpoints );

protected:
    static StorageBucket getBucket( StorageType storageType, StorageBucket defaultBucket = StorageBucket::Default );

private:
    std::mutex m_mutex;
    IDbClient& m_dbClient;
    CommonHttp::Http m_http;
    UploadResponses m_uploadResponses;

    struct DownloadInfo {
        DownloadResponse response;
        std::string error;
    };

    std::map< RequestId, DownloadInfo > m_downloadsInfo;
    std::unique_ptr< Dispatcher > m_dispatcher;
    JsonObject m_replacements;
};

}  // namespace CloudStorageInterface
}  // namespace Common
}  // namespace HubPlatform
