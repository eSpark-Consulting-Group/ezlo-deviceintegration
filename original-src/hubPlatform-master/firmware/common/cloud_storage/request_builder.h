#pragma once

#include "cloud_storage/cloud_storage_type.h"
#include "cloud_storage/controller_token.h"
#include "cloud_storage/resources.h"
#include "http/http_options.h"

namespace HubPlatform {
namespace Common {
namespace CloudStorageInterface {

class RequestBuilder final {
public:
    void httpOptionsUploadWithoutRedirect( CommonHttp::HttpOptions& httpOptions,
                                           const UploadFileData& data,
                                           StorageType storageType,
                                           StorageBucket storageBucket ) const;
    void httpOptionsUploadRedirect( CommonHttp::HttpOptions& httpOptions,
                                    const UploadFileData& data,
                                    StorageType storageType,
                                    StorageBucket storageBucket ) const;
    void httpOptionsUploadRedirectFileBody( CommonHttp::HttpOptions& httpOptions,
                                            const UploadFileData& data,
                                            const Json& metadata ) const;
    Errors httpOptionsUploadRedirectFinished( CommonHttp::HttpOptions& httpOptions,
                                              const ControllerToken& controllerToken,
                                              const UploadFileData& data,
                                              const Json& metadata ) const;

    void httpOptionsSetTimeouts( CommonHttp::HttpOptions& httpOptions ) const;
    void httpOptionsSuppressExpect( CommonHttp::HttpOptions& httpOptions ) const;
    void httpOptionsSetFileBody( const UploadFileData& data, CommonHttp::HttpOptions& httpOptions ) const;
    void httpOptionsSetFileMetaHeaders( const UploadFileData& data, CommonHttp::HttpOptions& httpOptions ) const;

    void httpOptionsSetDefaults( CommonHttp::HttpOptions& httpOptions,
                                 const char* httpMethod,
                                 const char* contentType ) const;

    void httpOptionsSetStorageTypes( StorageType& storageType,
                                     StorageBucket& storageBucket,
                                     CommonHttp::HttpOptions& httpOptions ) const;

    static void debugPrint( CommonHttp::HttpOptions& httpOptions, const std::string& token );

    void httpOptionsAuth( CommonHttp::HttpOptions& httpOptions, const std::string& token ) const;

    void httpOptionsSetUrl( CommonHttp::HttpOptions& httpOptions, const std::string& selectedUrl ) const;

    void httpOptionsSetXApp( CommonHttp::HttpOptions& httpOptions, const std::string& xapp ) const;

    Errors httpOptionsPrepareUploadLogs( CommonHttp::HttpOptions& httpOptions,
                                         const ControllerToken& controllerToken ) const;

    Errors httpOptionsPrepareUpload( CommonHttp::HttpOptions& httpOptions,
                                     const ControllerToken& controllerToken ) const;
    Errors httpOptionsPrepareUploadRedirect( CommonHttp::HttpOptions& httpOptions,
                                             const ControllerToken& controllerToken ) const;
    Errors httpOptionsPrepareFileBody( CommonHttp::HttpOptions& httpOptions,
                                       const ControllerToken& controllerToken ) const;

    Errors httpOptionsPrepareDownload( CommonHttp::HttpOptions& httpOptions,
                                       const ControllerToken& controllerToken ) const;
    Errors httpOptionsPrepareDownloadRedirect( CommonHttp::HttpOptions& httpOptions,
                                               const ControllerToken& controllerToken,
                                               const FileKey& fileKey ) const;
};

namespace Limits {
const int HttpResponseTimeoutMs = 5 * 60 * 1000;
const int HttpConnectionTimeoutMs = 30 * 1000;
}  // namespace Limits

}  // namespace CloudStorageInterface
}  // namespace Common
}  // namespace HubPlatform