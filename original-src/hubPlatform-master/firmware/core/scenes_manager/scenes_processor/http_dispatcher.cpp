#include "scenes_manager/scenes_processor/http_dispatcher.h"
#include "log_utils/ezlo_log.h"
#include "http/resources.h"
#include <curl/curl.h>
#include <openssl/ssl.h>
#include <sys/stat.h>

namespace HubPlatform {
namespace ScenesManager {

std::atomic_bool HttpDispatcher::s_initialized{ false };

void globalInit() {
    Log::debug( "http module globalInit()" );
    ::SSL_library_init();
    ::SSL_load_error_strings();
    ::ERR_load_BIO_strings();
    ::OpenSSL_add_all_algorithms();
    ::OpenSSL_add_all_digests();
    curl_global_init( CURL_GLOBAL_DEFAULT );
}

HttpDispatcher::HttpDispatcher( EventHandler&& receiveHandler, EventHandler&& timeoutHandler ) {
    bool expectedValue = false;
    if( s_initialized.compare_exchange_strong( expectedValue, true ) )
        globalInit();

    using namespace CommonHttp::Resources;
    m_http.on( HttpEvents::HttpDataReceived, std::move( receiveHandler ) );
    m_http.on( HttpEvents::HttpConnectionClosed, std::move( timeoutHandler ) );
}

HttpDispatcher::~HttpDispatcher() {
    if( m_http.joinable() ) {
        m_http.join();
        Log::trace( cc::trace( "HttpDispatch: Exit from join in plugin: " ) );
    }
}

HttpDispatcher::RequestId HttpDispatcher::request( const CommonHttp::HttpOptions& options ) {
    return m_http.request( "sceneAction", options );
}

}  // namespace ScenesManager
}  // namespace HubPlatform
