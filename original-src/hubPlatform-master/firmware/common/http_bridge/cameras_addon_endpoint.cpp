#include "http_bridge/cameras_addon_endpoint.h"
#include "http_bridge/ws_api_bridge.h"
#include "http_bridge/resources.h"

#include "base_utils/ezlodb.h"
#include "database_client/db_flags_names.h"
#include "common_utils/cpp14.h"
#include "database_client/dbid.h"
#include "base_utils/timeutils.h"

namespace HubPlatform {
namespace HttpBridge {

namespace SnapshotCacheParams {
const char* const ParamCached = "cached";
const char* const ParamRandom = "random";
const char* const ParamETag = "etag";
}  // namespace SnapshotCacheParams

const int CamerasAddonEndpoint::ConnectionTimeout;

CamerasAddonEndpoint::CamerasAddonEndpoint( ApiBridge& apiBridge )
: EndpointHandlers( { { LWS_CALLBACK_HTTP, &CamerasAddonEndpoint::onHttp },
                      { LWS_CALLBACK_HTTP_WRITEABLE, &CamerasAddonEndpoint::onHttpWriteable },
                      { LWS_CALLBACK_CLOSED_HTTP, &CamerasAddonEndpoint::onClosedHttp } } )
, m_endpointHandler( std::make_unique< EmptyCamerasEndpointHandler >() )
, m_api( apiBridge ) {}

EndpointPtr CamerasAddonEndpoint::makeEndpoint( ApiBridge& apiBridge ) {
    std::unique_ptr< CamerasAddonEndpoint > ptr( new CamerasAddonEndpoint( apiBridge ) );
    return EndpointPtr{ ptr.release() };
}

/** Http to WS API bridge. Implements methods:
 *  - /v1/addons/cameras/stream/snapshot/<stream_id>
 */
int CamerasAddonEndpoint::endpointCallback( struct lws* wsi,
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

void CamerasAddonEndpoint::setEndpointHandler( std::unique_ptr< ICamerasEndpointHandler >&& handler ) {
    m_endpointHandler = std::move( handler );
}

int CamerasAddonEndpoint::httpDummy( CallbackParams params ) {
    return HttpBridge::httpDummy( params );
}

int CamerasAddonEndpoint::onHttp( CallbackParams params, BufferPosition bufPos ) {
    using namespace Resources;
    auto* wsi = params.m_wsi;
    char* url = (char*) params.m_in;
    auto *start = bufPos.m_start, *&p = bufPos.m_p, *end = bufPos.m_end;

    SessionPtr session = m_api.startSession( wsi, Resources::sessionTypeHttp );
    if( !session.get() || !session->getData() )
        return -1;
    SessionData& sessionData = *session->getData();
    // emit parallel requests counter
    m_api.emit( Resources::Metrics::eventParallelRequests, Resources::EndpointHandlers::Cameras, session );

    if( !m_endpointHandler->isHandlerValid() ) {
        auto error = "no cameras addon";
        LOGGER_TRACE( error );
        m_endpointHandler->setErrorMessage( params, error );
        lws_callback_on_writable( params.m_wsi );
        return 0;
    }

    if( !url || std::strlen( url ) == 0 ) {
        auto error = "stream id not found";
        LOGGER_WARN( error );
        m_endpointHandler->setErrorMessage( params, error );
        lws_callback_on_writable( params.m_wsi );
        return 0;
    }

    lws_set_timeout( wsi, PENDING_TIMEOUT_HTTP_KEEPALIVE_IDLE, ConnectionTimeout );

    std::string urlString{ Resources::camerasAddonEndPoint };
    urlString += url;
    sessionData.getPath().assign( urlString );

    auto debugInfo = cc::debug( "Camera stream url:" ) + cc::u( url );
    session->extractHeaders( wsi );

    setEtagHeaderIfNeeded( sessionData );

    auto peerDebugInfo = sessionData.peerDebugInfo();
    debugInfo += ", ";
    debugInfo += peerDebugInfo;

    if( !isCachedParamInRequest( sessionData ) ) {
        debugInfo += ", no-cache";
    }

    LOGGER_INFO_RAW( debugInfo );
    m_api.emit( Resources::Metrics::eventRequestStarted, Resources::EndpointHandlers::Cameras, session );

    // if command is ok and all its data came in (for POST requests), call onMessage
    if( session->method == WSI_TOKEN_POST_URI ) {
        // wait for POST body completion, then call ApiBridge::onMessage().
    }
    else {
        /** Do we have result of auth here already? */
        getApiBridge().emit( Resources::eventAuthorization, Resources::EndpointHandlers::Cameras, session );

        if( !sessionData.isLoggedIn() && !Db::getFlag( OfflineAnonymousAccessFlagName ) ) {
            // answer should not be empty!
            sessionData.setAnswer( Json{ "error", "Authorization failed" } );
            sessionData.isPending( false );
            sessionData.notify();  // 401 Authorization failed
            // Stop executing the request and pass control to onHttpWritable, to return auth failure.
            return 0;
        }

        auto sessionCount = m_api.sessionCount();
        if( sessionCount > ApiBridge::parallelSessionsLimit() ) {
            m_api.emit( Resources::Metrics::eventRequestsLimit, Resources::EndpointHandlers::Cameras, session );

            const bool closeSession = true;
            ApiBridge::errorParallelSessionLimitReached( sessionData, sessionCount, start, p, end, closeSession );
            return 1;
        }

        // get <stream_id> here from an URL
        std::string tmpUrl = url;
        auto streamIdPos = tmpUrl.find_last_of( '/' ) + 1;
        if( streamIdPos == std::string::npos ) {
            auto error = "stream id not found";
            LOGGER_WARN( error );
            m_endpointHandler->setErrorMessage( params, error );
            lws_callback_on_writable( params.m_wsi );
            return 0;
        }
        auto streamId = tmpUrl.substr( streamIdPos );
        auto cached = isCachedParamInRequest( sessionData );

        auto etag = sessionData.getHeaderValue( SnapshotCacheParams::ParamETag );

        m_endpointHandler->getSnapshot( params, streamId, cached, etag );
    }

    return 0;
}

bool CamerasAddonEndpoint::isCachedParamInRequest( SessionData& sessionData ) {
    auto cachedParam = sessionData.getCommand().findParam( SnapshotCacheParams::ParamCached );
    if( !cachedParam.empty() ) {
        return atoi( cachedParam.c_str() ) == 1;
    }
    return true;
}

void CamerasAddonEndpoint::setEtagHeaderIfNeeded( SessionData& sessionData ) {
    auto requestId = sessionData.getCommand().findParam( SnapshotCacheParams::ParamRandom );
    if( requestId.empty() ) {
        requestId = sessionData.getHeaderValue( SnapshotCacheParams::ParamETag );
    }
    if( requestId.empty() ) {
        requestId = "autogen_" + db::DbId::genOID();
    }
    LOGGER_TRACE( ", requestId: " + requestId );
    sessionData.setId( requestId );
}

int CamerasAddonEndpoint::writeHttpHeaders( SessionData& sessionData,
                                            int httpStatus,
                                            const char* contentType,
                                            uint8_t* start,
                                            uint8_t*& p,
                                            uint8_t* end,
                                            bool closeSession,
                                            CallbackParams params ) {
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

    auto timePoint = m_endpointHandler->getCachedEndTimestamp( params );
    if( timePoint > 0 ) {
        auto timePointString = TimeUtils::convertTimeToHttpExpires( timePoint );
        if( lws_add_http_header_by_name(
                wsi, (uint8_t*) "expired:", (uint8_t*) timePointString.c_str(), timePointString.size(), &p, end ) ) {
            return 1;
        }
    }

    if( lws_finalize_http_header( wsi, &p, end ) ) {
        return 1;
    }
#else

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

    if( lws_write( wsi, start, lws_ptr_diff( p, start ), static_cast< lws_write_protocol >( writeProtocolFlags ) ) < 0 )
        return -1;

    if( closeSession && lws_http_transaction_completed( wsi ) ) {
        LOGGER_ERROR( "Failed to finalize transaction: " + cc::num16( (uint64_t) wsi ) );
        return -1;
    }

    sessionData.headersSent( true );
    return 1;
}

int CamerasAddonEndpoint::onHttpWriteable( CallbackParams params, BufferPosition bufPos ) {
    auto* wsi = params.m_wsi;
    auto *start = bufPos.m_start, *&p = bufPos.m_p, *end = bufPos.m_end;

    int n = LWS_WRITE_HTTP;

    SessionPtr session = m_api.findSession( wsi );
    if( !session || !session->getData() ) {
        return -1;
    }
    auto& sessionData = *session->getData();
    if( !sessionData.headersSent() ) {
        // Send headers here
        auto cached = isCachedParamInRequest( sessionData );
        if( !sessionData.isLoggedIn() && !Db::getFlag( OfflineAnonymousAccessFlagName ) ) {
            LOGGER_TRACE_RAW( cc::trace( "Wasn't authorized; wsi: " ) + cc::num016( (uint64_t) wsi ) );
            m_api.emit( Resources::Metrics::eventRequestUnauthorized, Resources::EndpointHandlers::Cameras, session );

            const bool closeSession = true;
            auto result =
                writeHttpHeaders( sessionData, HTTP_STATUS_UNAUTHORIZED, "", start, p, end, closeSession, params );
            sessionData.headersSent( true );
            if( result != 0 ) {
                LOGGER_ERROR( "writeHttpHeaders returned: " + cc::num10( result ) );
                return result;
            }
            return 1;
        }
        else {
            if( !m_endpointHandler->getErrorMessage( params ).empty() ) {
                const char* contentType = "text/html";
                writeHttpHeaders( sessionData, HTTP_STATUS_NOT_FOUND, contentType, start, p, end, false, params );
            }
            else if( cached && !sessionData.getHeaderValue( "if-modified-since" ).empty() &&
                     !isModifiedCachedImage( params, sessionData ) ) {
                const char* contentType = "text/html";
                writeHttpHeaders( sessionData, HTTP_STATUS_NOT_MODIFIED, contentType, start, p, end, false, params );
            }
            else {
                const char* contentType = "image/jpeg";
                writeHttpHeaders( sessionData, HTTP_STATUS_OK, contentType, start, p, end, false, params );
            }
            sessionData.headersSent( true );
            lws_callback_on_writable( wsi );
            return 0;
        }
    }
    auto error = m_endpointHandler->getErrorMessage( params );
    size_t messageSize;
    if( error.empty() ) {
        messageSize = m_endpointHandler->getFileSize( params );
    }
    else {
        messageSize = error.size();
    }
    // send body here
    /* write using portions of a size defined by lws */
    long toWriteLimit = end - p - 1;

    if( messageSize == 0 || (int) ( messageSize - sessionData.bytesWritten() ) < (int) toWriteLimit )
        n = LWS_WRITE_HTTP_FINAL;
    if( error.empty() ) {
        size_t offset = m_endpointHandler->readFile( params, (char*) p, toWriteLimit );
        p += offset;
        if( n == LWS_WRITE_HTTP_FINAL ) {
            LOGGER_TRACE_PRINT( "httpCallback / http_writeable: progress: %d/%u, portion: %u, wsi: %p",
                                (int) ( sessionData.bytesWritten() + offset ),
                                messageSize,
                                offset,
                                wsi );
        }
    }
    else {
        p += lws_snprintf( (char*) p, end - p, "%s", error.c_str() );
    }
    auto toWrite = lws_ptr_diff( p, start );
    auto bytesWritten = lws_write( wsi, (uint8_t*) start, toWrite, static_cast< lws_write_protocol >( n ) );
    sessionData.addBytesWritten( bytesWritten );

    if( bytesWritten != toWrite ) {
        LOGGER_TRACE( "bytesWritten != toWrite" );
        m_api.emit( Resources::Metrics::eventPayloadTx, Resources::EndpointHandlers::Cameras, session );
        m_api.emit( Resources::Metrics::eventRequestFailed, Resources::EndpointHandlers::Cameras, session );

        return 1;
    }

    if( n == LWS_WRITE_HTTP_FINAL ) {
        m_api.emit( Resources::Metrics::eventPayloadTx, Resources::EndpointHandlers::Cameras, session );
        m_api.emit( Resources::Metrics::eventRequestFinished, Resources::EndpointHandlers::Cameras, session );
        if( lws_http_transaction_completed( wsi ) )
            return -1;
    }
    else if( !sessionData.isPending() ) {
        lws_callback_on_writable( wsi );
    }
    return 0;
}

bool CamerasAddonEndpoint::isModifiedCachedImage( CallbackParams& params, SessionData& sessionData ) {
    auto ifModifiedDateStrValue = sessionData.getHeaderValue( "if-modified-since" );
    auto ifModifiedDate = TimeUtils::convertHttpExpiresToTime( ifModifiedDateStrValue );
    if( ifModifiedDate > 0 ) {
        auto cachedTimestamp = m_endpointHandler->getCachedTimestamp( params );
        auto cachedEndTimestamp = m_endpointHandler->getCachedEndTimestamp( params );
        auto isModified = ifModifiedDate < cachedTimestamp;
        auto isExpired = ifModifiedDate > cachedEndTimestamp;
        return isModified || isExpired;
    }
    return false;
}

int CamerasAddonEndpoint::onClosedHttp( CallbackParams params, BufferPosition /*bufPos*/ ) {
    LOGGER_TRACE_PRINT( "httpCallback / closed_http: wsi: %p", params.m_wsi );

    if( m_endpointHandler ) {
        m_endpointHandler->removeFile( params );
    }

    auto session = m_api.findSession( params.m_wsi );
    if( session && session->getData() ) {
        auto& sessionData = *session->getData();
        std::string requestId = sessionData.getId();

        if( !sessionData.headersSent() ) {
            LOGGER_DEBUG( "Session closed without being handled, requestId: " + requestId );
            m_api.emit( Resources::Metrics::eventRequestUnhandled, Resources::EndpointHandlers::Cameras, session );
        }
        else {
            LOGGER_INFO_RAW( "Request finished: requestId: " + requestId );
        }
    }

    m_api.finishSession( params.m_wsi );
    m_api.emit( Resources::Metrics::eventParallelRequests, Resources::EndpointHandlers::Cameras, session );
    return httpDummy( params );
}

size_t ICamerasEndpointHandler::getFileSize( CallbackParams params ) {
    std::lock_guard< std::mutex > lock( m_mutex );
    auto it = m_files.find( params.m_user );
    if( it == m_files.end() ) {
        LOGGER_DEBUG( "file not found" );
        return 0;
    }
    return it->second.fileSize;
}

size_t ICamerasEndpointHandler::readFile( CallbackParams params, char* dst, size_t size ) {
    std::lock_guard< std::mutex > lock( m_mutex );
    auto it = m_files.find( params.m_user );
    if( it == m_files.end() ) {
        LOGGER_DEBUG( "file not found" );
        return 0;
    }
    auto& file = it->second.file;
    if( file == nullptr || feof( file.get() ) ) {
        return 0;
    }
    return fread( dst, 1, size, file.get() );
}

void ICamerasEndpointHandler::removeFile( CallbackParams params ) {
    std::lock_guard< std::mutex > lock( m_mutex );
    m_files.erase( params.m_user );
}

void ICamerasEndpointHandler::callbackOnWritable( CallbackParams params ) {
    lws_callback_on_writable( params.m_wsi );
}

std::string ICamerasEndpointHandler::getErrorMessage( HttpBridge::CallbackParams params ) {
    std::lock_guard< std::mutex > lock( m_mutex );
    auto it = m_files.find( params.m_user );
    if( it == m_files.end() ) {
        return "no file found";
    }
    return it->second.error;
}

void ICamerasEndpointHandler::setErrorMessage( HttpBridge::CallbackParams params, const std::string& errorMessage ) {
    std::lock_guard< std::mutex > lock( m_mutex );
    m_files[ params.m_user ].error = errorMessage;
}

void EmptyCamerasEndpointHandler::getSnapshot( CallbackParams /*params*/,
                                               const std::string& /*streamId*/,
                                               bool /*cached*/,
                                               const std::string& /*requestId*/ ) {}

bool EmptyCamerasEndpointHandler::isHandlerValid() {
    return false;
}

time_t EmptyCamerasEndpointHandler::getCachedEndTimestamp( HttpBridge::CallbackParams params ) {
    return 0;
}

time_t EmptyCamerasEndpointHandler::getCachedTimestamp( HttpBridge::CallbackParams params ) {
    return 0;
}

}  // namespace HttpBridge
}  // namespace HubPlatform
