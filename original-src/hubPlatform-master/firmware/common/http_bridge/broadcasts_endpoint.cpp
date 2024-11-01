#include "http_bridge/broadcasts_endpoint.h"
#include "http_bridge/ws_api_bridge.h"
#include "http_bridge/resources.h"
#include "http_bridge/endpoint_handlers.h"

#include "base_utils/ezlodb.h"
#include "database_client/db_flags_names.h"

#include <cassert>

namespace HubPlatform {
namespace HttpBridge {

BroadcastsEndpoint::BroadcastsEndpoint( ApiBridge& apiBridge )
: EndpointHandlers( {
      { LWS_CALLBACK_HTTP, &BroadcastsEndpoint::onHttp },
      { LWS_CALLBACK_HTTP_WRITEABLE, &BroadcastsEndpoint::onHttpWriteable },
      { LWS_CALLBACK_CLOSED_HTTP, &BroadcastsEndpoint::onClosedHttp },
  } )
, m_api( apiBridge ) {}

EndpointPtr BroadcastsEndpoint::makeEndpoint( ApiBridge& apiBridge ) {
    std::unique_ptr< BroadcastsEndpoint > ptr( new BroadcastsEndpoint( apiBridge ) );
    return EndpointPtr{ ptr.release() };
}

/** Http to WS API bridge. Implements methods:
 *  - /v1/broadcast
 */
int BroadcastsEndpoint::endpointCallback( struct lws* wsi,
                                          enum lws_callback_reasons reason,
                                          void* user,
                                          void* in,
                                          size_t len ) {
    uint8_t buf[ LWS_PRE + LWS_RECOMMENDED_MIN_HEADER_SPACE ];
    uint8_t *start = &buf[ LWS_PRE ], *p = start, *end = &buf[ sizeof( buf ) - LWS_PRE - 1 ];
    CallbackParams params{ wsi, reason, user, in, len };
    BufferPosition bufPos{ start, p, end };

    const auto it = m_handlers.find( reason );
    if( it != m_handlers.end() ) {
        const auto handler = it->second;
        return ( this->*handler )( params, bufPos );
    }
    return httpDummy( params );
}

int BroadcastsEndpoint::httpDummy( CallbackParams params ) {
    return HttpBridge::httpDummy( params );
}

int BroadcastsEndpoint::onHttp( CallbackParams params, BufferPosition bufPos ) {
    auto* wsi = params.m_wsi;
    int len = params.m_len;
    auto *start = bufPos.m_start, *&p = bufPos.m_p, *end = bufPos.m_end;

    auto session = m_api.startSession( wsi, Resources::sessionTypeEventStream );
    if( !session || !session->getData() )
        return 1;
    session->extractHeaders( wsi );
    m_api.emit( Resources::Metrics::eventRequestStarted, Resources::EndpointHandlers::Broadcasts, session );
    // emit parallel requests counter
    m_api.emit( Resources::Metrics::eventParallelRequests, Resources::EndpointHandlers::Broadcasts, session );

    auto sessionData = session->getData();
    auto peerDebugInfo = sessionData->peerDebugInfo();

    LOGGER_DEBUG_RAW( peerDebugInfo );

    auto sessionCount = m_api.sessionCount();
    // >= instead of > for GET/POST requests: leave one connection to queries
    if( sessionCount >= ApiBridge::parallelSessionsLimit() ) {
        m_api.emit( Resources::Metrics::eventRequestsLimit, Resources::EndpointHandlers::Broadcasts, session );

        const bool closeSession = true;
        ApiBridge::errorParallelSessionLimitReached( *session->getData(), sessionCount, start, p, end, closeSession );
        return 1;
    }

    /* No timeout */
#ifdef USE_LWS_3_2
    lws_http_mark_sse( struct lws * wsi );
#else
    // In lws 3.1 this doesn't allow to hold connection forever, it lasts only around 30 seconds and is closed
    // by an implementation. fixed in lws version 3.2.
    lws_set_timeout( wsi, NO_PENDING_TIMEOUT, 0 );
#endif
    // authorize here
    m_api.emit( Resources::eventListenBroadcasts, "SSE", session );

    lws_callback_on_writable( wsi );

    return 0;
}

int BroadcastsEndpoint::onHttpWriteable( CallbackParams params, BufferPosition bufPos ) {
    auto* wsi = params.m_wsi;
    auto len = params.m_len;
    auto *start = bufPos.m_start, *&p = bufPos.m_p, *end = bufPos.m_end;

    SessionPtr pss = m_api.findSession( wsi );
    if( !pss || !pss->getData() )
        return httpDummy( params );
    auto& sessionData = *pss->getData();
    if( sessionData.isPending() )
        return 0;

    if( !sessionData.headersSent() ) {
        if( !sessionData.isLoggedIn() && !Db::getFlag( OfflineAnonymousAccessFlagName ) ) {
            LOGGER_TRACE_RAW( cc::trace( "Wasn't authorized; wsi: " ) + cc::num016( (uint64_t) wsi ) );
            const bool closeSession = true;
            return ApiBridge::writeHttpStatusHeaders(
                sessionData, HTTP_STATUS_UNAUTHORIZED, "", start, p, end, closeSession );
        }
        else {
            /* SSE requires a response with this content-type */
            if( lws_add_http_common_headers(
                    wsi, HTTP_STATUS_OK, "text/event-stream", LWS_ILLEGAL_HTTP_CONTENT_LEN, &p, end ) )
                return 1;

            lws_add_http_header_by_name( wsi, (uint8_t*) "access-control-allow-origin:", (uint8_t*) "*", 1, &p, end );

            // ('Cache-Control: no-cache');
            lws_add_http_header_by_name(
                wsi, (uint8_t*) "cache-control:", (uint8_t*) "no-cache", sizeof( "no-cache" ) - 1, &p, end );

            if( lws_finalize_write_http_header( wsi, start, &p, end ) )
                return 1;

            sessionData.headersSent( true );

            lws_callback_on_writable( wsi );
            return 0;
        }
    }

    assert( sessionData.headersSent() == true );
    int n = LWS_WRITE_HTTP;
    if( sessionData.hasSerializedAnswer() || sessionData.nextBroadcast() ) {
        static char messageEndMarker[] = "\n";
        static char messagePrefix[] = "data:";
        constexpr const size_t reservedSpace = ( sizeof( messageEndMarker ) - 1 ) + ( sizeof( messagePrefix ) - 1 ) + 1;

        /* write using portions of a size defined by lws */
        long toWrite = end - p - 1 - reservedSpace;
        std::string portion = sessionData.getSerializedAnswer( toWrite );
        size_t size = portion.size();
        if( size == 0 ) {
            if( sessionData.nextBroadcast() ) {
                portion = sessionData.getSerializedAnswer( toWrite );
                size = portion.size();
            }
        }

        p += lws_snprintf( (char*) p, end - p, "%s%s\n", messagePrefix, portion.c_str() );

        if( size == 0 || (int) size < toWrite ) {
            n = LWS_WRITE_HTTP_FINAL;
            p += lws_snprintf( (char*) p, end - p, "\n" );
        }

        // Never use LWS_WRITE_HTTP_FINAL for event stream
        auto bytes = lws_write(
            wsi, (uint8_t*) start, lws_ptr_diff( p, start ), static_cast< lws_write_protocol >( LWS_WRITE_HTTP ) );

        if( bytes == -1 ) {
            LOGGER_TRACE_RAW( cc::trace( "Write attempt failed. Drop session " ) + cc::num016( (uint64_t) wsi ) );
            // Return -1 to drop the connection. Broadcasts streams can be dropped only when the new portion to
            //  write appears. Just user disconnect doesn't do this.
            return -1;
        }
        if( bytes != lws_ptr_diff( p, start ) ) {
            return 1;
        }

        if( n == LWS_WRITE_HTTP_FINAL ) {
            if( sessionData.nextBroadcast() ) {
                lws_callback_on_writable( wsi );
            }
            return 0;
        }
        else {
            // never finalize eventStream transaction
            lws_callback_on_writable( wsi );
            return 0;
        }
    }

    return 0;
}

int BroadcastsEndpoint::onClosedHttp( CallbackParams params, BufferPosition bufPos ) {
    LOGGER_TRACE_PRINT( "callback broadcasts / closed_http: %u, wsi: %p", params.m_len, params.m_wsi );

    SessionPtr session = m_api.findSession( params.m_wsi );
    if( session ) {
        m_api.emit( Resources::Metrics::eventRequestFinished, Resources::EndpointHandlers::Broadcasts, session );
    }

    m_api.finishSession( params.m_wsi );
    m_api.emit( Resources::Metrics::eventParallelRequests, Resources::EndpointHandlers::Broadcasts, session );

    return httpDummy( params );
}

}  // namespace HttpBridge
}  // namespace HubPlatform