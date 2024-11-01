#include "http_bridge/ws_api_bridge.h"
#include "http_bridge/resources.h"
#include "http_bridge/addons_endpoint.h"
#include "log_utils/ezlo_log.h"
#include "json_utils/json_utils.h"
#include "tiny/tiny_consolecolors.h"
#include "main_http_endpoint.h"
#include "broadcasts_endpoint.h"
#include "cameras_addon_endpoint.h"
#include "http_bridge/session_manager.h"
#include "http_bridge/http_metrics.h"
#include "common_utils/cpp14.h"
#include <memory>

#include <utility>

/** Implementation related to (HTTP Server v0.10)[https://confluence.mios.com/pages/viewpage.action?pageId=33395301] */

namespace HubPlatform {
namespace HttpBridge {

ApiBridge::ApiBridge() {
    m_metrics = std::make_unique< HttpServerMetrics >();
    m_mainHttp = MainHttpEndpoint::makeEndpoint( *this );
    m_broadcasts = BroadcastsEndpoint::makeEndpoint( *this );
    m_camerasAddon = CamerasAddonEndpoint::makeEndpoint( *this );

    m_sessionManager = std::make_unique< SessionManager >();
}

/** Http to WS API bridge. Implements methods:
 *  - /v1/request
 *  - /v1/method/[name-of-command-with-slashes]/#version
 */
int ApiBridge::httpCallback( struct lws* wsi, enum lws_callback_reasons reason, void* user, void* in, size_t len ) {
    auto& apiBridge = getApiBridge();
    auto& endPointPtr = apiBridge.mainHttpEndpoint();
    if( endPointPtr ) {
        return endPointPtr->endpointCallback( wsi, reason, user, in, len );
    }

    return lws_callback_http_dummy( wsi, reason, user, in, len );
}

bool ApiBridge::getResponseError( const Json& answer, Json& error ) {
    return answer.is_object() && json_get_obj( answer, "error", error ) && !error.empty();
}

int ApiBridge::writeHttpStatusHeaders( SessionData& sessionData,
                                       int httpStatus,
                                       const char* contentType,
                                       uint8_t* start,
                                       uint8_t*& p,
                                       uint8_t* end,
                                       bool closeSession ) {
    auto* wsi = sessionData.getWsi();

#define LWS_NO_HTTP2
#ifdef LWS_NO_HTTP2
    if( lws_add_http_common_headers( wsi,
                                     httpStatus,
                                     contentType ? contentType : "application/json",
                                     LWS_ILLEGAL_HTTP_CONTENT_LEN, /* no content len */
                                     &p,
                                     end ) ) {
        return 1;
    }
    if( lws_finalize_http_header( wsi, &p, end ) ) {
        return 1;
    }
#else
    // Recomendations from https://libwebsockets.org/lws-api-doc-master/html/md_READMEs_README_8coding.html
    // from section "Code Requirements for HTTP/2 compatibility"
    // do not help to implement valid http/2 response and lead to http-server crashes.

    lws_add_http_header_status( wsi, httpStatus, &p, end );
    if( contentType && *contentType )
        lws_add_http_header_by_name(
            wsi, (const uint8_t*) "content-type", (uint8_t*) contentType, strlen( contentType ), &p, end );
    if( lws_finalize_http_header( wsi, &p, end ) )
        return 1;
#endif

    unsigned writeProtocolFlags = LWS_WRITE_HTTP_HEADERS;
    if( closeSession )
        writeProtocolFlags |= LWS_WRITE_H2_STREAM_END;

    lws_write( wsi, start, lws_ptr_diff( p, start ), static_cast< lws_write_protocol >( writeProtocolFlags ) );

    if( closeSession && lws_http_transaction_completed( wsi ) )
        return -1;

    sessionData.headersSent( true );
    return 1;
}

/** Http to WS API bridge. Implements methods:
 *  - /v1/broadcast
 */
int ApiBridge::httpEventStreamCallback( struct lws* wsi,
                                        enum lws_callback_reasons reason,
                                        void* user,
                                        void* in,
                                        size_t len ) {
    auto& apiBridge = getApiBridge();
    auto& endPointPtr = apiBridge.broadcastsEndpoint();
    if( endPointPtr ) {
        return endPointPtr->endpointCallback( wsi, reason, user, in, len );
    }

    return lws_callback_http_dummy( wsi, reason, user, in, len );
}

int ApiBridge::httpCamerasCallback( struct lws* wsi,
                                    enum lws_callback_reasons reason,
                                    void* user,
                                    void* in,
                                    size_t len ) {
    auto& apiBridge = getApiBridge();
    auto& endPointPtr = apiBridge.camerasAddonEndpoint();
    if( endPointPtr ) {
        return endPointPtr->endpointCallback( wsi, reason, user, in, len );
    }

    return lws_callback_http_dummy( wsi, reason, user, in, len );
}

int ApiBridge::httpAddonsCallback( struct lws* wsi,
                                   enum lws_callback_reasons reason,
                                   void* user,
                                   void* in,
                                   size_t len ) {
    if( auto endpoint = getApiBridge().addonsEndpoint() ) {
        return endpoint->endpointCallback( wsi, reason, user, in, len );
    }
    return lws_callback_http_dummy( wsi, reason, user, in, len );
}

const lws_protocols* ApiBridge::getProtocols( unsigned& length ) {
    static struct lws_protocols protocols[] = {
        { Resources::apiBridgeProtocol,
          HttpBridge::ApiBridge::httpCallback,
          sizeof( HttpBridge::Session ),
          0,
          0,
          &getApiBridge() },
        { Resources::eventStreamProtocol,
          HttpBridge::ApiBridge::httpEventStreamCallback,
          sizeof( HttpBridge::Session ),
          0,
          0,
          &getApiBridge() },
        { Resources::camerasAddonProtocol,
          HttpBridge::ApiBridge::httpCamerasCallback,
          sizeof( HttpBridge::Session ),
          0,
          0,
          &getApiBridge() },
        { Resources::httpLinksProtocol,
          HttpBridge::ApiBridge::httpAddonsCallback,
          sizeof( void* ),
          0,
          0,
          (void*) ( Resources::httpLinksProtocol + 0 ) },
        { Resources::addonsProtocol, HttpBridge::ApiBridge::httpAddonsCallback, sizeof( void* ), 0, 0, nullptr },
    };
    length = sizeof( protocols ) / sizeof( protocols[ 0 ] );
    return protocols;
}

const lws_http_mount& ApiBridge::getHttpMount() {
    using namespace Resources;

    static struct lws_http_mount mountCamerasSnapshot = {
        /* .mount_next */ nullptr,              /* linked-list "next" */
        /* .mountpoint */ camerasAddonEndPoint, /* mountpoint URL */
        /* .origin */ nullptr,                  /* protocol */
        /* .def */ nullptr,
        /* .protocol */ camerasAddonProtocol,
        /* .cgienv */ nullptr,
        /* .extra_mimetypes */ nullptr,
        /* .interpret */ nullptr,
        /* .cgi_timeout */ 0,
        /* .cache_max_age */ 0,
        /* .auth_mask */ 0,
        /* .cache_reusable */ 0,
        /* .cache_revalidate */ 0,
        /* .cache_intermediaries */ 0,
        /* .origin_protocol */ LWSMPRO_CALLBACK,                  /* dynamic */
        /* .mountpoint_len */ sizeof( camerasAddonEndPoint ) - 1, /* char count */
        /* .basic_auth_login_file */ nullptr,
    };

    // temporarily add this mount-point as a static one, should be served by UI HTTP extensions
    static struct lws_http_mount httpLinksMountPoint = {
        /* .mount_next */ &mountCamerasSnapshot,                  /* linked-list "next" */
        /* .mountpoint */ /* addonsEndPoint */ httpLinksEndPoint, /* mountpoint URL */
        /* .origin */ nullptr,                                    /* protocol */
        /* .def */ nullptr,
        /* .protocol */ httpLinksProtocol,
        /* .cgienv */ nullptr,
        /* .extra_mimetypes */ nullptr,
        /* .interpret */ nullptr,
        /* .cgi_timeout */ 0,
        /* .cache_max_age */ 0,
        /* .auth_mask */ 0,
        /* .cache_reusable */ 0,
        /* .cache_revalidate */ 0,
        /* .cache_intermediaries */ 0,
        /* .origin_protocol */ LWSMPRO_CALLBACK,               /* dynamic */
        /* .mountpoint_len */ sizeof( httpLinksEndPoint ) - 1, /* char count */
        /* .basic_auth_login_file */ nullptr,
    };

    static struct lws_http_mount addonsMountPoint = {
        /* .mount_next */ &httpLinksMountPoint, /* linked-list "next" */
        /* .mountpoint */ addonsEndPoint,       /* mountpoint URL */
        /* .origin */ nullptr,                  /* protocol */
        /* .def */ nullptr,
        /* .protocol */ addonsProtocol,
        /* .cgienv */ nullptr,
        /* .extra_mimetypes */ nullptr,
        /* .interpret */ nullptr,
        /* .cgi_timeout */ 0,
        /* .cache_max_age */ 0,
        /* .auth_mask */ 0,
        /* .cache_reusable */ 0,
        /* .cache_revalidate */ 0,
        /* .cache_intermediaries */ 0,
        /* .origin_protocol */ LWSMPRO_CALLBACK,            /* dynamic */
        /* .mountpoint_len */ sizeof( addonsEndPoint ) - 1, /* char count */
        /* .basic_auth_login_file */ nullptr,
    };

    static struct lws_http_mount eventStreamMountPoint = {
        /* .mount_next */ &addonsMountPoint,   /* linked-list "next" */
        /* .mountpoint */ eventStreamEndPoint, /* mountpoint URL */
        /* .origin */ nullptr,                 /* protocol */
        /* .def */ nullptr,
        /* .protocol */ eventStreamProtocol,
        /* .cgienv */ nullptr,
        /* .extra_mimetypes */ nullptr,
        /* .interpret */ nullptr,
        /* .cgi_timeout */ 0,
        /* .cache_max_age */ 0,
        /* .auth_mask */ 0,
        /* .cache_reusable */ 0,
        /* .cache_revalidate */ 0,
        /* .cache_intermediaries */ 0,
        /* .origin_protocol */ LWSMPRO_CALLBACK,                 /* dynamic */
        /* .mountpoint_len */ sizeof( eventStreamEndPoint ) - 1, /* char count */
        /* .basic_auth_login_file */ nullptr,
    };

    static struct lws_http_mount mountHttpApiBridge = {
        /* .mount_next */ &eventStreamMountPoint, /* linked-list "next" */
        /* .mountpoint */ apiBridgeEndPoint,      /* mountpoint URL */
        /* .origin */ nullptr,                    /* protocol */
        /* .def */ nullptr,
        /* .protocol */ apiBridgeProtocol,
        /* .cgienv */ nullptr,
        /* .extra_mimetypes */ nullptr,
        /* .interpret */ nullptr,
        /* .cgi_timeout */ 0,
        /* .cache_max_age */ 0,
        /* .auth_mask */ 0,
        /* .cache_reusable */ 0,
        /* .cache_revalidate */ 0,
        /* .cache_intermediaries */ 0,
        /* .origin_protocol */ LWSMPRO_CALLBACK,               /* dynamic */
        /* .mountpoint_len */ sizeof( apiBridgeEndPoint ) - 1, /* char count */
        /* .basic_auth_login_file */ nullptr,
    };

    return mountHttpApiBridge;
}

SessionPtr ApiBridge::startSession( lws* wsi, const char* type ) {
    return m_sessionManager->startSession( wsi, type );
}

void ApiBridge::finishSession( lws* wsi ) {
    m_sessionManager->finishSession( wsi );
}

SessionPtr ApiBridge::findSession( lws* wsi ) const {
    return m_sessionManager->findSession( wsi );
}

bool ApiBridge::registerExternalSession() {
    m_sessionManager->incrementExternalSessions();
    LOGGER_TRACE_RAW( "Register external HTTP session: " + cc::num10( m_sessionManager->sessionCount() ) +
                      ", external: " + cc::num10( m_sessionManager->externalSessions() ) );
    return m_sessionManager->sessionCount() < parallelSessionsLimit();
}

void ApiBridge::unregisterExternalSession() {
    m_sessionManager->decrementExternalSessions();
    LOGGER_TRACE_RAW( "Unregister external HTTP session: " + cc::num10( m_sessionManager->sessionCount() ) +
                      ", external: " + cc::num10( m_sessionManager->externalSessions() ) );
}

void ApiBridge::sendBroadcast( const std::string& msg ) {
    if( m_sessionManager->eventStreamsCount() == 0 )
        return;

    m_sessionManager->forEachSession( msg, []( const std::string& msg, const SessionPtr& ptr ) {
        if( ptr && ptr->getData() ) {
            auto& session = *ptr;
            auto& sessionData = *session.getData();

            if( session.sessionType() == Session::SessionType::EVENT_STREAM ) {
                session.postMessage( msg );
            }
        }
    } );
}

size_t ApiBridge::sessionCount() const {
    return m_sessionManager->sessionCount();
}

int& ApiBridge::parallelSessionsLimit() {
    static int limit = Resources::PARALLEL_SESSIONS_LIMIT;
    return limit;
}

int ApiBridge::errorParallelSessionLimitReached( SessionData& sessionData,
                                                 size_t sessionCount,
                                                 uint8_t* start,
                                                 uint8_t*& p,
                                                 uint8_t* end,
                                                 bool closeSession ) {
    // raise httpStatus 429
    Log::error( cc::error( "ApiBridge: parallel sessions limit reached: " ) + cc::num10( sessionCount ) );
    const auto httpStatus = Resources::HTTP_STATUS_CONTROLLER_PARALLEL_LIMIT_REACHED;
    return writeHttpStatusHeaders( sessionData, httpStatus, "", start, p, end, closeSession );
}

int& ApiBridge::requestDurationLimit() {
    static int limit = Resources::REQUEST_DURATION_LIMIT;
    return limit;
}

GenericSenderPtr ApiBridge::createBridgePeer( SessionPtr session ) {
    // Do not assign ref_retain_release-based object to a retain_release_ptr: leads to inability to reach ref-count=0,
    // memory-leak
    GenericSenderPtr ptr;
    ptr.attach( new ApiBridgePeer( std::move( session ) ) );
    return ptr.get();
}

void ApiBridge::resetHandlers( ApiBridgeHandlersPtr ptr ) {
    m_handlers = std::move( ptr );

    // resets all listeners set before
    off( Resources::eventMessage, nullptr );
    off( Resources::eventListenBroadcasts, nullptr );
    off( Resources::eventAuthorization, nullptr );

    if( m_handlers.get() ) {
        on( Resources::eventMessage, [this]( const std::string& handler, SessionPtr session ) {
            m_handlers->message( handler, std::move( session ) );
        } );
        on( Resources::eventListenBroadcasts, [this]( const std::string& handler, SessionPtr session ) {
            m_handlers->listenBroadcasts( handler, std::move( session ) );
        } );
        on( Resources::eventAuthorization, [this]( const std::string& handler, SessionPtr session ) {
            m_handlers->authorization( handler, std::move( session ) );
        } );
    }

    resetMetricsHandlers();
}

void ApiBridge::resetMetricsHandlers() {
    LOGGER_DEBUG( cc::debug( "Reset metrics handlers for ApiBridge" ) );

    using namespace Resources::Metrics;
    const std::vector< std::string > metricsEvents = {
        eventRequestStarted, eventRequestFinished,     eventRequestFailed,  eventRequestsLimit,    eventPayloadRx,
        eventPayloadTx,      eventRequestUnauthorized, eventRequestTimeOut, eventRequestUnhandled, eventParallelRequests
    };

    using EventName = std::string;
    using MetricsIncrementor = void ( HttpServerMetrics::* )();
    using MetricsAppender = void ( HttpServerMetrics::* )( size_t );

    auto handlerFactory = [this]( const std::string& eventName ) {
        auto handler = [this, eventName]( const std::string& handler, SessionPtr session ) {
            LOGGER_TRACE_RAW( "ApiBridge event " + eventName + ", handler: " + handler );

            static const std::map< EventName, MetricsIncrementor > metricsDispatcher = {
                { eventRequestStarted, &HttpServerMetrics::incrementReceivedRequest },
                { eventRequestFinished, &HttpServerMetrics::incrementHandledRequest },
                { eventRequestFailed, &HttpServerMetrics::incrementErrorRequest },
                { eventRequestUnauthorized, &HttpServerMetrics::incrementWrongAuthRequest },
                { eventRequestsLimit, &HttpServerMetrics::incrementRequestsLimitReached },
                { eventRequestTimeOut, &HttpServerMetrics::incrementRequestTimedOut },
                { eventRequestUnhandled, &HttpServerMetrics::incrementUnhandledRequest },
            };

            if( auto metrics = dynamic_cast< HttpServerMetrics* >( m_metrics.get() ) ) {
                {
                    auto foundIt = metricsDispatcher.find( eventName );
                    if( foundIt != metricsDispatcher.end() ) {
                        const auto handler = foundIt->second;
                        ( metrics->*handler )();
                        return;
                    }
                }

                if( session ) {
                    if( eventName == Resources::Metrics::eventPayloadRx ) {
                        auto payloadRx = session->getData()->getPostData().size();
                        LOGGER_TRACE_RAW( "payload received: " + cc::num10( payloadRx ) );
                        metrics->appendRxBytes( payloadRx );
                    }
                    else if( eventName == Resources::Metrics::eventPayloadTx ) {
                        size_t payloadTx = session->getData()->bytesWritten();
                        LOGGER_TRACE_RAW( "payload transferred: " + cc::num10( payloadTx ) );
                        metrics->appendTxBytes( payloadTx );
                    }
                    else if( eventName == Resources::Metrics::eventParallelRequests ) {
                        const size_t parallelRequests = m_sessionManager->sessionCount();
                        metrics->parallelRequests( parallelRequests );
                    }
                }
            }
        };
        return handler;
    };

    for( auto& event : metricsEvents ) {
        off( event, nullptr );
        on( event, handlerFactory( event ) );
    }
}

void ApiBridge::resetAddonsEndpoint( AddonsEndpoint* addonsEndpoint ) {
    m_addons = addonsEndpoint;
}

EndpointPtr& ApiBridge::mainHttpEndpoint() {
    return m_mainHttp;
}

EndpointPtr& ApiBridge::broadcastsEndpoint() {
    return m_broadcasts;
}

EndpointPtr& ApiBridge::camerasAddonEndpoint() {
    return m_camerasAddon;
}

AddonsEndpoint* ApiBridge::addonsEndpoint() {
    return m_addons;
}

/** @brief Writes 100 continue response, should be followed by real headers finalized block. */
int ApiBridge::writeHttpExpect100Response( lws* wsi ) {
    uint8_t buf[ LWS_PRE + LWS_RECOMMENDED_MIN_HEADER_SPACE ];
    uint8_t *start = &buf[ LWS_PRE ], *p = start, *end = &buf[ sizeof( buf ) - 1 ];
    const int EXPECT_100_CONTINUE = 100;
    lws_add_http_header_status( wsi, EXPECT_100_CONTINUE, &p, end );
    return lws_write( wsi, start, p - start, LWS_WRITE_HTTP_HEADERS );
}

Json ApiBridge::getMetrics() const {
    return m_metrics->toJson();
}

ApiBridge& getApiBridge() {
    static ApiBridge bridge;
    return bridge;
}

ApiBridgePeer::ApiBridgePeer( SessionPtr ptr )
: m_session( std::move( ptr ) ) {}

ApiBridgePeer::~ApiBridgePeer() = default;

bool ApiBridgePeer::sendMessage( const std::string& msg, int nOpCode ) {
    if( m_session && m_session->getData() ) {
        SessionData& sessionData = *m_session->getData();
        sessionData.setSerializedAnswer( msg );
        sessionData.isPending( false );
        sessionData.notify();
        return true;
    }
    return false;
}

bool ApiBridgePeer::sendMessageText( const std::string& msg ) {
    return sendMessage( msg, 0 );
}

bool ApiBridgePeer::sendMessageBinary( const std::string& msg ) {
    return sendMessage( msg, 0 );
}

ApiBridgeMessage::ApiBridgeMessage( GenericSenderPtr ptr )
: m_sender( std::move( ptr ) ) {}

ApiBridgeMessage::~ApiBridgeMessage() {
    m_sender = nullptr;
}

GenericSenderPtr ApiBridgeMessage::getNetworkObject() const {
    return m_sender;
}

}  // namespace HttpBridge
}  // namespace HubPlatform
