#pragma once

#include "http/http.h"
#include "jsonrpc/error_codes.h"

#include <string>
#include <functional>
#include <map>
#include <mutex>
#include <chrono>

namespace HubPlatform {
namespace UpdateManager {

class CustomPluginDownloader final {
public:
    class CustomPluginDownloaderBusy : public Errors::Exception {
    public:
        using Errors::Exception::Exception;
    };

    CustomPluginDownloader();

    using SuccessCallback = std::function< void() >;
    using FailCallback = std::function< void( const std::string& errorDescription ) >;

    using DownloadRequestId = CommonHttp::Http::RequestId;
    DownloadRequestId downloadFileAsync( const std::string& url,
                                         const std::string& filePath,
                                         SuccessCallback successCb,
                                         FailCallback failCb );

private:
    CommonHttp::Http m_http;
    std::mutex m_downlodRequestsMutex;

    using Callbacks = std::pair< SuccessCallback, FailCallback >;
    std::map< CommonHttp::Http::RequestId, Callbacks > m_downlodRequests;

    const int m_maxConcurrentDownloadRequests = 5;
    std::chrono::milliseconds m_maxDownloadRequestDuration = std::chrono::minutes( 5 );
    std::chrono::milliseconds m_connectionTimeout = std::chrono::seconds( 30 );
};

}  // namespace UpdateManager
}  // namespace HubPlatform
