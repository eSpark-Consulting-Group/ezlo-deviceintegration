
#include "update_manager/um_custom_plugin_downloader.h"

#include "http/http_events.h"
#include "json_utils/json_utils.h"

#include <utility>

namespace HubPlatform {
namespace UpdateManager {

CustomPluginDownloader::CustomPluginDownloader() {
    using namespace CommonHttp::Resources;
    m_http.on(
        HttpEvents::HttpConnectionClosed, [this]( const std::string& /*eventHandler*/, CommonHttp::EventPtr eventPtr ) {
            if( !eventPtr.get() )
                return;

            auto message = eventPtr->toJson();

            int reasonCode;
            json_get( message, std::vector< Json >{ "reason", "code" }, reasonCode );
            auto requestId = json_get_str( message, "id" );
            auto code = json_get_int( message, "code" );

            std::unique_lock< std::mutex > lock( m_downlodRequestsMutex );

            auto it = m_downlodRequests.find( requestId );
            if( it != m_downlodRequests.end() ) {
                if( static_cast< CommonHttp::Reason >( reasonCode ) == CommonHttp::Reason::NONE && code == 200 ) {
                    lock.unlock();
                    it->second.first();
                    lock.lock();
                }
                else {
                    std::string errorDescription;
                    json_get( message, std::vector< Json >{ "reason", "text" }, errorDescription );

                    lock.unlock();
                    it->second.second( errorDescription );
                    lock.lock();
                }

                auto iterator = m_downlodRequests.find( requestId );
                if( iterator != m_downlodRequests.end() ) {
                    m_downlodRequests.erase( iterator );
                }
            }
        } );
}

CustomPluginDownloader::DownloadRequestId CustomPluginDownloader::downloadFileAsync( const std::string& url,
                                                                                     const std::string& filePath,
                                                                                     SuccessCallback successCb,
                                                                                     FailCallback failCb ) {
    std::lock_guard< std::mutex > lock( m_downlodRequestsMutex );
    if( m_downlodRequests.size() >= m_maxConcurrentDownloadRequests ) {
        throw CustomPluginDownloaderBusy( Errors::CUSTOM_PLUGIN_DOWNLOADER_BUSY );
    }

    CommonHttp::HttpOptions httpOptions;

    httpOptions.url = url;
    httpOptions.type = "GET";
    httpOptions.handler = "empty";
    httpOptions.receivedDataFilePath = filePath;
    httpOptions.timeoutMs = m_maxDownloadRequestDuration.count();
    httpOptions.connectTimeoutMs = m_connectionTimeout.count();
    httpOptions.skipSecurity = true;

    auto requestId = m_http.request( "asyncFileDownloader", httpOptions );
    m_downlodRequests.emplace( requestId, std::make_pair( successCb, failCb ) );

    return requestId;
}
}  // namespace UpdateManager
}  // namespace HubPlatform
