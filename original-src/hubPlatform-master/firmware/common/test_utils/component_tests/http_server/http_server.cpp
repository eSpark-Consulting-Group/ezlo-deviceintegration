#include "test_utils/component_tests/http_server/http_server.h"
#include "test_utils/component_tests/http_server/reflect_endpoint.h"
#include "http_bridge/session_manager.h"
#include "http_bridge/utils.h"
#include "log_utils/ezlo_log.h"
#include "json_utils/json_fwd.h"
#include "common_utils/cpp14.h"
#include "tiny/tiny_consolecolors.h"
#include "tiny/tiny_utils.h"
#include <libwebsockets.h>

namespace HubPlatform {
namespace Tests {

using LwsDeleter = void ( * )( lws_context* );
using LwsPtr = std::unique_ptr< lws_context, LwsDeleter >;
std::string pathToFolder( const char* file );

class HttpServerImpl {
public:
    HttpServerImpl();

    const std::string& makeFullPath( const char* relative, const char* baseFolder );
    void updateMountPaths( lws_http_mount* mount, const char* baseFolder );

    static HttpBridge::EndpointPtr& reflectEndpoint();
    static HttpBridge::ISessionManager& sessionManager();

public:
    lws_context_creation_info m_info;
    LwsPtr m_context;

    std::map< std::string, std::string > m_fullPathMap;
};

HttpServerImpl::HttpServerImpl()
: m_context( nullptr, &lws_context_destroy ) {}

const std::string& HttpServerImpl::makeFullPath( const char* relative, const char* baseFolder ) {
    if( relative && baseFolder ) {
        std::string folder = baseFolder;
        auto result = m_fullPathMap.emplace( relative, folder + relative );

        if( result.second ) {
            return result.first->second;
        }
    }
    static std::string empty;
    return empty;
}

void HttpServerImpl::updateMountPaths( lws_http_mount* mount, const char* baseFolder ) {
    if( mount && baseFolder ) {
        if( mount->basic_auth_login_file && *mount->basic_auth_login_file ) {
            mount->basic_auth_login_file = makeFullPath( mount->basic_auth_login_file, baseFolder ).c_str();
        }
        if( mount->origin && *mount->origin && mount->origin_protocol == LWSMPRO_FILE ) {
            const auto& fullPath = makeFullPath( mount->origin, baseFolder );
            mount->origin = fullPath.c_str();
        }

        if( mount->mount_next )
            return updateMountPaths( const_cast< lws_http_mount* >( mount->mount_next ), baseFolder );
    }
}

HttpBridge::EndpointPtr& HttpServerImpl::reflectEndpoint() {
    static HttpBridge::EndpointPtr ptr = std::make_unique< ReflectEndpoint >( sessionManager() );
    return ptr;
}

HttpBridge::ISessionManager& HttpServerImpl::sessionManager() {
    static HttpBridge::SessionManager manager;
    return manager;
}

class HttpServer::Impl : public HttpServerImpl {};

HttpServer::HttpServer( /** server context */ )
: m_pimpl( new Impl() ) {}

void HttpServer::startInternal() {
    init();

    if( !isRunning() ) {
        if( m_thread.joinable() )
            m_thread.join();
    }

    m_thread = std::thread( &HttpServer::runServer, this );
    std::this_thread::sleep_for( std::chrono::milliseconds{ 200 } );
}

void HttpServer::stopInternal() {
    if( m_thread.joinable() ) {
        m_thread.join();
    }
}

struct pss {
    using KeyValueVector = HttpBridge::Utils::KeyValueVector;

public:
    lws_token_indexes method = WSI_TOKEN_GET_URI;
    KeyValueVector knownHeaders;
    KeyValueVector uriParams;

    char path[ 128 ];
    int times;
    int budget;
    int content_lines;

public:
    unsigned extractHeaders( lws* wsi );
};

unsigned pss::extractHeaders( lws* wsi ) {
    using namespace HubPlatform::HttpBridge::Utils;
    this->knownHeaders = HttpBridge::Utils::extractHeaders( wsi );
    this->uriParams = HttpBridge::Utils::extractUriParams( wsi );
    return this->knownHeaders.size();
}

static int interrupted;

static int callback_dynamic_http( struct lws* wsi,
                                  enum lws_callback_reasons reason,
                                  void* user,
                                  void* in,
                                  size_t len ) {
    struct pss* pss = (struct pss*) user;
    uint8_t buf[ LWS_PRE + 2048 ], *start = &buf[ LWS_PRE ], *p = start, *end = &buf[ sizeof( buf ) - LWS_PRE - 1 ];
    time_t t;
    int n;
    auto* url = static_cast< const char* >( in );

    switch( reason ) {
    case LWS_CALLBACK_HTTP: {
        lws_snprintf( pss->path, sizeof( pss->path ), "%s", url );
        Log::debug( cc::debug( "Dyn: Url: " ) + url );
        pss->extractHeaders( wsi );
        auto httpStatusIt = std::find_if(
            pss->uriParams.begin(), pss->uriParams.end(), []( const pss::KeyValueVector::value_type& item ) {
                return item.first == "httpStatus";
            } );

        auto httpStatus = HTTP_STATUS_OK;
        if( httpStatusIt != pss->uriParams.end() ) {
            if( unsigned statusPassed = std::stoi( httpStatusIt->second ) )
                httpStatus = static_cast< http_status >( statusPassed );
        }

        if( lws_add_http_common_headers( wsi,
                                         httpStatus,
                                         "text/html",
                                         LWS_ILLEGAL_HTTP_CONTENT_LEN, /* no content len */
                                         &p,
                                         end ) )
            return 1;
        if( lws_finalize_write_http_header( wsi, start, &p, end ) )
            return 1;

        pss->times = 0;
        pss->budget = atoi( (char*) in + 1 );
        pss->content_lines = 0;
        if( !pss->budget )
            pss->budget = 10;

        /* write the body separately */
        lws_callback_on_writable( wsi );

        return 0;
    }

    case LWS_CALLBACK_HTTP_WRITEABLE:

    {
        if( !pss || pss->times > pss->budget )
            break;

        n = LWS_WRITE_HTTP;
        if( pss->times == pss->budget )
            n = LWS_WRITE_HTTP_FINAL;

        if( !pss->times ) {
            t = time( NULL );
            p += lws_snprintf( (char*) p,
                               end - p,
                               "<html>"
                               "<head><meta charset=utf-8 "
                               "http-equiv=\"Content-Language\" "
                               "content=\"en\"/></head><body>"
                               "<img src=\"/libwebsockets.org-logo.svg\">"
                               "<br>Dynamic content for '%s' from mountpoint."
                               "<br>Time: %s<br><br>"
                               "</body></html>",
                               pss->path,
                               ctime( &t ) );
        }
        else {
            while( lws_ptr_diff( end, p ) > 80 )
                p += lws_snprintf(
                    (char*) p, end - p, "%d.%d: this is some content... ", pss->times, pss->content_lines++ );

            p += lws_snprintf( (char*) p, end - p, "<br><br>" );
        }

        pss->times++;
        if( lws_write( wsi, (uint8_t*) start, lws_ptr_diff( p, start ), static_cast< lws_write_protocol >( n ) ) !=
            lws_ptr_diff( p, start ) )
            return 1;

        if( n == LWS_WRITE_HTTP_FINAL ) {
            if( lws_http_transaction_completed( wsi ) )
                return -1;
        }
        else
            lws_callback_on_writable( wsi );

        return 0;
    }
    default:
        break;
    }

    return lws_callback_http_dummy( wsi, reason, user, in, len );
}

static int callback_reflect_http( struct lws* wsi,
                                  enum lws_callback_reasons reason,
                                  void* user,
                                  void* in,
                                  size_t len ) {
    if( HttpServerImpl::reflectEndpoint() == nullptr ) {
        LOGGER_ERROR( "ReflectEndpoint is null" );

        HttpBridge::CallbackParams tmp( wsi, reason, user, in, len );
        return -1;
    }
    return HttpServerImpl::reflectEndpoint()->endpointCallback( wsi, reason, user, in, len );
}

static struct lws_protocols protocols[] = {
    { "http", lws_callback_http_dummy, 0, 0 },
    { "reflect-demo", callback_reflect_http, sizeof( struct pss ), 0 },
    { "dyn-demo", callback_dynamic_http, sizeof( struct pss ), 0 },
    { NULL, NULL, 0, 0 } /* terminator */
};

static struct lws_http_mount mountReflect = {
    /* .mount_next */ NULL,       /* linked-list "next" */
    /* .mountpoint */ "/reflect", /* mountpoint URL */
    /* .origin */ NULL,           /* protocol */
    /* .def */ NULL,
    /* .protocol */ "reflect-demo",
    /* .cgienv */ NULL,
    /* .extra_mimetypes */ NULL,
    /* .interpret */ NULL,
    /* .cgi_timeout */ 0,
    /* .cache_max_age */ 0,
    /* .auth_mask */ 0,
    /* .cache_reusable */ 0,
    /* .cache_revalidate */ 0,
    /* .cache_intermediaries */ 0,
    /* .origin_protocol */ LWSMPRO_CALLBACK, /* dynamic */
    /* .mountpoint_len */ 8,                 /* char count */
    /* .basic_auth_login_file */ NULL,
};

/* override the default mount for /dyn in the URL space */
static struct lws_http_mount mountDyn = {
    /* .mount_next */ &mountReflect, /* linked-list "next" */
    /* .mountpoint */ "/dyn",        /* mountpoint URL */
    /* .origin */ NULL,              /* protocol */
    /* .def */ NULL,
    /* .protocol */ "dyn-demo",
    /* .cgienv */ NULL,
    /* .extra_mimetypes */ NULL,
    /* .interpret */ NULL,
    /* .cgi_timeout */ 0,
    /* .cache_max_age */ 0,
    /* .auth_mask */ 0,
    /* .cache_reusable */ 0,
    /* .cache_revalidate */ 0,
    /* .cache_intermediaries */ 0,
    /* .origin_protocol */ LWSMPRO_CALLBACK, /* dynamic */
    /* .mountpoint_len */ 4,                 /* char count */
    /* .basic_auth_login_file */ NULL,
};

static struct lws_http_mount mountAuth = {
    /* .mount_next */ &mountDyn,                /* linked-list "next" */
    /* .mountpoint */ "/auth",                  /* mountpoint URL */
    /* .origin */ "resources/mount_point/auth", /* serve from dir */
    /* .def */ "index.html",                    /* default filename */
    /* .protocol */ NULL,
    /* .cgienv */ NULL,
    /* .extra_mimetypes */ NULL,
    /* .interpret */ NULL,
    /* .cgi_timeout */ 0,
    /* .cache_max_age */ 0,
    /* .auth_mask */ 0,
    /* .cache_reusable */ 0,
    /* .cache_revalidate */ 0,
    /* .cache_intermediaries */ 0,
    /* .origin_protocol */ LWSMPRO_FILE, /* files in a dir */
    /* .mountpoint_len */ 5,             /* char count */
    /* .basic_auth_login_file */ "resources/certs/.basic_auth",
};

/* default mount serves the URL space from /mount_point */
static struct lws_http_mount mountMain = {
    /* .mount_next */ &mountAuth,          /* linked-list "next" */
    /* .mountpoint */ "/",                 /* mountpoint URL */
    /* .origin */ "resources/mount_point", /* serve from dir */
    /* .def */ "index.html",               /* default filename */
    /* .protocol */ NULL,
    /* .cgienv */ NULL,
    /* .extra_mimetypes */ NULL,
    /* .interpret */ NULL,
    /* .cgi_timeout */ 0,
    /* .cache_max_age */ 0,
    /* .auth_mask */ 0,
    /* .cache_reusable */ 0,
    /* .cache_revalidate */ 0,
    /* .cache_intermediaries */ 0,
    /* .origin_protocol */ LWSMPRO_FILE, /* files in a dir */
    /* .mountpoint_len */ 1,             /* char count */
    /* .basic_auth_login_file */ NULL,
};

std::string pathToFolder( const char* file ) {
    std::string theFile{ file ? file : "" };
    const auto lastPos = theFile.find_last_of( "/", std::string::npos );
    return theFile.substr( 0, lastPos );
}

void HttpServer::init() {
    if( !m_pimpl )
        throw std::runtime_error( "Pimpl" );

    lws_context_creation_info& info = m_pimpl->m_info;
    //    lws_context*& context = pimpl->m_context;

    const char* p;
    int n = 0, logs = LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE;
    //#ifdef _DEBUG
    LOGGER_WARN( "DEV: set debug logging for lws" );
    logs |= LLL_DEBUG;
    //#endif

#ifdef DEBUG
    logs = 2047;
    LOGGER_WARN( "DEV: set all logging for lws: " + cc::num10( logs ) );
#endif

    lws_set_log_level( logs, nullptr );

    Log::info( cc::info( "http_bridge/LWS minimal http server | visit http://localhost:" ) +
               std::to_string( getPort() ) );

    memset( &info, 0, sizeof( info ) ); /* otherwise uninitialized garbage */
    /* Flag removed to enable expect: 100-continue handling: if the flag is set, Content Security Policy headers are
     * added after HTTP/1.1 100 header. */
    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT | LWS_SERVER_OPTION_EXPLICIT_VHOSTS;

    info.max_http_header_pool = 10;

    m_pimpl->m_context = LwsPtr( lws_create_context( &info ), &lws_context_destroy );
    auto* context = m_pimpl->m_context.get();
    if( !context ) {
        throw std::runtime_error( "lws init failed" );
    }

    std::string theFile = __FILE__;
    auto lastPos = theFile.find_last_of( "/", std::string::npos );
    std::string thisFolder = lastPos < std::string::npos ? theFile.substr( 0, lastPos + 1 ) : std::string();

    m_pimpl->updateMountPaths( const_cast< lws_http_mount* >( &mountMain ), thisFolder.c_str() );

    /* http on port */

    info.port = getPort();
    info.protocols = protocols;
    info.mounts = &mountMain;
    info.vhost_name = "http";

    if( !lws_create_vhost( context, &info ) )
        throw std::runtime_error( "Failed to create tls vhost" );

    /* https on m_securePort */
    info.port = getSecurePort();
    info.error_document_404 = "/404.html";

    info.ssl_cert_filepath = m_pimpl->makeFullPath( "resources/certs/localhost_100y.cert", thisFolder.c_str() ).c_str();
    info.ssl_private_key_filepath =
        m_pimpl->makeFullPath( "resources/certs/localhost_100y.key", thisFolder.c_str() ).c_str();
    info.vhost_name = "localhost";

    if( !lws_create_vhost( context, &info ) ) {
        throw std::runtime_error( "Failed to create tls vhost" );
    }
}

void HttpServer::runServer() {
    int n = 1;
    while( n >= 0 && isRunning() )
        n = lws_service( m_pimpl->m_context.get(), 1000 );
}

unsigned HttpServer::getPort() const {
    return m_port;
}
unsigned HttpServer::getSecurePort() const {
    return m_securePort;
}

void HttpServer::configure( const Configuration& conf ) {}

HttpServer::~HttpServer() {
    stop();

    if( m_pimpl ) {
        delete m_pimpl;
        m_pimpl = nullptr;
    }
}

void HttpServer::setPort( unsigned int port ) {
    m_port = port;
}

void HttpServer::setSecurePort( unsigned int port ) {
    m_securePort = port;
}

}  // namespace Tests
}  // namespace HubPlatform