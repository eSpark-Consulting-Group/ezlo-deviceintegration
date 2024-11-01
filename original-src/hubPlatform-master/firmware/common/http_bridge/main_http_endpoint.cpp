#include "http_bridge/main_http_endpoint.h"
#include "http_bridge/ws_api_bridge.h"
#include "http_bridge/resources.h"

#include "base_utils/ezlodb.h"
#include "database_client/db_flags_names.h"
#include "database_client/dbid.h"

namespace HubPlatform {
namespace HttpBridge {

MainHttpEndpoint::MainHttpEndpoint( ApiBridge& apiBridge )
: EndpointHandlers( { { LWS_CALLBACK_HTTP, &MainHttpEndpoint::onHttp },
                      { LWS_CALLBACK_HTTP_WRITEABLE, &MainHttpEndpoint::onHttpWriteable },
                      { LWS_CALLBACK_HTTP_BODY, &MainHttpEndpoint::onHttpBody },
                      { LWS_CALLBACK_HTTP_BODY_COMPLETION, &MainHttpEndpoint::onHttpBodyCompletion },
                      { LWS_CALLBACK_CLOSED_HTTP, &MainHttpEndpoint::onClosedHttp },
                      { LWS_CALLBACK_TIMER, &MainHttpEndpoint::onTimer } } )
, m_api( apiBridge ) {}

EndpointPtr MainHttpEndpoint::makeEndpoint( ApiBridge& apiBridge ) {
    std::unique_ptr< MainHttpEndpoint > ptr( new MainHttpEndpoint( apiBridge ) );
    return EndpointPtr{ ptr.release() };
}

/** Http to WS API bridge. Implements methods:
 *  - /v1/request
 *  - /v1/method/[name-of-command-with-slashes]/#version
 */
int MainHttpEndpoint::endpointCallback( struct lws* wsi,
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

int httpDummy( CallbackParams& params ) {
    return lws_callback_http_dummy( params.m_wsi, params.m_reason, params.m_user, params.m_in, params.m_len );
}

int MainHttpEndpoint::httpDummy( CallbackParams params ) {
    return HttpBridge::httpDummy( params );
}

int MainHttpEndpoint::onHttp( CallbackParams params, BufferPosition bufPos ) {
    auto* wsi = params.m_wsi;
    char* url = (char*) params.m_in;
    auto *start = bufPos.m_start, *&p = bufPos.m_p, *end = bufPos.m_end;

    SessionPtr session = m_api.startSession( wsi, Resources::sessionTypeHttp );
    if( !session.get() || !session->getData() )
        return -1;
    SessionData& sessionData = *session->getData();
    // emit parallel requests counter
    m_api.emit( Resources::Metrics::eventParallelRequests, Resources::EndpointHandlers::Main, session );

    if( url ) {
        std::string urlString{ Resources::apiBridgeEndPoint };
        urlString += url;
        sessionData.getPath().assign( urlString );
    }

    session->extractHeaders( wsi );
    setEtagHeaderIfNeeded( sessionData );

    auto debugInfo = cc::debug( "ApiBridge: Url:" ) + url;
    auto peerDebugInfo = sessionData.peerDebugInfo();

    debugInfo += " ,";
    debugInfo += peerDebugInfo;

    LOGGER_INFO_RAW( debugInfo );

    // if command is ok and all its data came in (for POST requests), call onMessage
    if( session->method == WSI_TOKEN_POST_URI ) {
        // wait for POST body completion, then call ApiBridge::onMessage().
        if( sessionData.hasHeader( "expect:", "100-continue" ) ) {
            ApiBridge::writeHttpExpect100Response( wsi );
        }
        // Exit here to prevent onWritable callback call
        m_api.emit( Resources::Metrics::eventRequestStarted, Resources::EndpointHandlers::Main, session );
        return 0;
    }
    else {
        auto sessionCount = m_api.sessionCount();
        if( sessionCount > ApiBridge::parallelSessionsLimit() ) {
            const bool closeSession = true;
            ApiBridge::errorParallelSessionLimitReached( sessionData, sessionCount, start, p, end, closeSession );
            m_api.emit( Resources::Metrics::eventRequestsLimit, Resources::EndpointHandlers::Main, session );

            return 1;
        }

        auto parseResult = sessionData.parseGetCommand( sessionData.getPath() );
        if( !parseResult.first ) {
            LOGGER_DEBUG_RAW( cc::debug( "Command parsing result: raise httpStatus 400. Error: " ) +
                              cc::error( parseResult.second ) );

            m_api.emit( Resources::Metrics::eventRequestFailed, Resources::EndpointHandlers::Main, session );
            // Send header 400 and exit
            const bool closeSession = true;
            return ApiBridge::writeHttpStatusHeaders(
                sessionData, HTTP_STATUS_BAD_REQUEST, "", start, p, end, closeSession );
        }

        HttpBridgeCommand& command = sessionData.getCommand();
        if( command.method().empty() ) {
            // raise httpStatus 400
            LOGGER_TRACE_RAW( cc::trace( "Command method is empty: raise httpStatus 400" ) );

            m_api.emit( Resources::Metrics::eventRequestFailed, Resources::EndpointHandlers::Main, session );
            // Send header 400 and exit
            const bool closeSession = true;
            return ApiBridge::writeHttpStatusHeaders(
                sessionData, HTTP_STATUS_BAD_REQUEST, "", start, p, end, closeSession );
        }

        auto delayedVal = sessionData.getCommand().findParam( "__delayed" );
        if( !delayedVal.empty() )
            sessionData.delayed( atoi( delayedVal.c_str() ) );

        // All data came in, call onMessage()
        m_api.emit( Resources::eventMessage, Resources::httpMethodGet, session );
        m_api.emit( Resources::Metrics::eventRequestStarted, Resources::EndpointHandlers::Main, session );

        // use timer to limit request duration
        auto microSeconds = ApiBridge::requestDurationLimit() * 1000;
        lws_set_timer_usecs( wsi, microSeconds );
    }

    lws_callback_on_writable( wsi );

    return 0;
}

void MainHttpEndpoint::setEtagHeaderIfNeeded( SessionData& sessionData ) const {
    auto requestId = sessionData.getId();
    if( requestId.empty() ) {
        requestId = db::DbId::genOID();
        sessionData.setId( requestId );
    }
}

int MainHttpEndpoint::onHttpWriteable( CallbackParams params, BufferPosition bufPos ) {
    auto* wsi = params.m_wsi;
    auto *start = bufPos.m_start, *&p = bufPos.m_p, *end = bufPos.m_end;
    int n = LWS_WRITE_HTTP;

    SessionPtr session = m_api.findSession( wsi );
    if( !session || !session->getData() )
        return -1;
    auto& sessionData = *session->getData();
    bool emitFinishEvents = true;

    if( !sessionData.hasSerializedAnswer() ) {
        bool isPending = sessionData.isPending();
        if( !isPending ) {
            int indent = Log::getLocalIndent();

            auto foundVal = sessionData.getCommand().findParam( "__indent" );
            if( !foundVal.empty() )
                indent = std::stoi( foundVal );

            sessionData.buildResponse( indent );
        }
    }

    if( sessionData.hasSerializedAnswer() ) {
        if( !sessionData.headersSent() ) {
            // Send headers here
            if( !sessionData.isLoggedIn() && !Db::getFlag( OfflineAnonymousAccessFlagName ) ) {
                LOGGER_TRACE_RAW( cc::trace( "Wasn't authorized; wsi: " ) + cc::num016( (uint64_t) wsi ) );
                m_api.emit( Resources::Metrics::eventRequestUnauthorized, Resources::EndpointHandlers::Main, session );

                const bool closeSession = true;
                return ApiBridge::writeHttpStatusHeaders(
                    sessionData, HTTP_STATUS_UNAUTHORIZED, "", start, p, end, closeSession );
            }
            else {
                // for POST request check json body and raise http status 400 on empty or bad json here
                if( session->method == WSI_TOKEN_POST_URI ) {
                    const auto& request = sessionData.postDataJson();
                    if( request.empty() || !request.is_object() || json_get_str( request, "method" ).empty() ) {
                        bool closeSession = true;
                        m_api.emit(
                            Resources::Metrics::eventRequestFailed, Resources::EndpointHandlers::Main, session );

                        Log::error( cc::error( "ApiBridge: Bad request" ) );
                        return ApiBridge::writeHttpStatusHeaders(
                            sessionData, HTTP_STATUS_BAD_REQUEST, "", start, p, end, closeSession );
                    }
                }

                const auto& answer = sessionData.getAnswer();

                Json error;
                if( ApiBridge::getResponseError( answer, error ) ) {
                    int code = json_get_int( error, "code", 0 );
                    std::string data = json_get_str( error, "data" );
                    std::string message = json_get_str( error, "message" );
                    if( code != 0 ) {
                        // Raise error 500
                        bool closeSession = false;
                        Log::error( cc::error( "ApiBridge: Web-socket error in response: " ) + cc::j( error ) );
                        m_api.emit(
                            Resources::Metrics::eventRequestFailed, Resources::EndpointHandlers::Main, session );

                        ApiBridge::writeHttpStatusHeaders(
                            sessionData, HTTP_STATUS_INTERNAL_SERVER_ERROR, "", start, p, end, closeSession );
                        lws_callback_on_writable( wsi );
                        return 0;
                    }
                }
                ApiBridge::writeHttpStatusHeaders(
                    sessionData, HTTP_STATUS_OK, "application/json", start, p, end, false );

                lws_callback_on_writable( wsi );
                return 0;
            }
        }

        // send body here
        /* write using portions of a size defined by lws */
        long toWriteLimit = end - p - 1;
        std::string portion = sessionData.getSerializedAnswer( toWriteLimit );
        size_t size = portion.size();

        if( size == 0 || (int) size < toWriteLimit )
            n = LWS_WRITE_HTTP_FINAL;

        if( n == LWS_WRITE_HTTP_FINAL ) {
            LOGGER_TRACE_PRINT(
                "httpCallback / http_writeable: toWriteLimit: %u/%d, wsi: %p", size, (int) toWriteLimit, wsi );
        }

        p += lws_snprintf( (char*) p, end - p, "%s", portion.c_str() );

        auto toWrite = lws_ptr_diff( p, start );
        auto bytesWritten = lws_write( wsi, (uint8_t*) start, toWrite, static_cast< lws_write_protocol >( n ) );
        sessionData.addBytesWritten( bytesWritten );

        if( bytesWritten != toWrite ) {
            m_api.emit( Resources::Metrics::eventRequestFailed, Resources::EndpointHandlers::Main, session );
            m_api.emit( Resources::Metrics::eventPayloadTx, Resources::EndpointHandlers::Main, session );
            return 1;
        }
        else {
            if( n == LWS_WRITE_HTTP_FINAL ) {
                m_api.emit( Resources::Metrics::eventRequestFinished, Resources::EndpointHandlers::Main, session );
                m_api.emit( Resources::Metrics::eventPayloadTx, Resources::EndpointHandlers::Main, session );
                emitFinishEvents = false;
            }
        }
    }

    if( n == LWS_WRITE_HTTP_FINAL ) {
        if( emitFinishEvents ) {
            m_api.emit( Resources::Metrics::eventRequestFinished, Resources::EndpointHandlers::Main, session );
            m_api.emit( Resources::Metrics::eventPayloadTx, Resources::EndpointHandlers::Main, session );
        }
        if( lws_http_transaction_completed( wsi ) )
            return -1;
    }
    else if( !sessionData.isPending() )
        lws_callback_on_writable( wsi );

    return 0;
}

int MainHttpEndpoint::onHttpBody( CallbackParams params, BufferPosition bufPos ) {
    auto* wsi = params.m_wsi;
    char* data = (char*) params.m_in;
    const auto len = params.m_len;
    if( data && len > 0 ) {
        SessionPtr pss = m_api.findSession( wsi );
        if( !pss || !pss->getData() )
            return -1;

        auto& buffer = pss->getData()->getPostData();
        buffer.insert( buffer.end(), data, data + len );
        LOGGER_TRACE_PRINT( "httpCallback / http_body: %u", len );
    }
    return httpDummy( params );
}

int MainHttpEndpoint::onHttpBodyCompletion( CallbackParams params, BufferPosition bufPos ) {
    auto* wsi = params.m_wsi;
    char* data = (char*) params.m_in;
    const auto len = params.m_len;
    auto *start = bufPos.m_start, *&p = bufPos.m_p, *end = bufPos.m_end;

    SessionPtr session = m_api.findSession( wsi );
    if( !session || !session->getData() )
        return -1;

    auto& sessionData = *session->getData();
    auto& buffer = sessionData.getPostData();
    if( data && len > 0 ) {
        buffer.insert( buffer.end(), data, data + len );
        LOGGER_TRACE_PRINT( "httpCallback / http_body_completion: %u, wsi: %p", len, wsi );
    }
    LOGGER_TRACE_PRINT( "httpCallback / http_body_completion: Total body size: %u, wsi: %p", buffer.size(), wsi );

    if( !sessionData.headersSent() ) {
        auto sessionCount = m_api.sessionCount();
        // TODO: Decide should I exit here, because this request already been started without this condition. Let's
        // reject new requests only.
        if( sessionCount > ApiBridge::parallelSessionsLimit() ) {
            const bool closeSession = true;
            ApiBridge::errorParallelSessionLimitReached( sessionData, sessionCount, start, p, end, closeSession );
            return 1;
        }
    }

    auto& postData = sessionData.getPostData();
    if( !postData.empty() ) {
        if( sessionData.parsePostData() ) {
            auto delayedVal = sessionData.getCommand().findParam( "__delayed" );
            if( !delayedVal.empty() )
                sessionData.delayed( atoi( delayedVal.c_str() ) );

            const auto& postData = sessionData.postDataJson();
            bool isValid =
                !postData.empty() && postData.is_object() && json_get_str( postData, "method" ).empty() == false;
            if( isValid ) {
                m_api.emit( Resources::eventMessage, Resources::httpMethodPost, session );
            }
            else {
                m_api.emit( Resources::Metrics::eventRequestFailed, Resources::EndpointHandlers::Main, session );
                ApiBridge::writeHttpStatusHeaders( sessionData, HTTP_STATUS_BAD_REQUEST, "", start, p, end, true );
                return 1;
            }

            // use timer to limit request duration
            auto microSeconds = ApiBridge::requestDurationLimit() * 1000;
            lws_set_timer_usecs( wsi, microSeconds );
        }
    }

    lws_callback_on_writable( wsi );

    return 0;
}

int MainHttpEndpoint::onClosedHttp( CallbackParams params, BufferPosition bufPos ) {
    LOGGER_TRACE_PRINT( "httpCallback / closed_http: wsi: %p", params.m_wsi );
    auto session = m_api.findSession( params.m_wsi );
    if( session && session->getData() ) {
        auto& sessionData = *session->getData();
        const std::string requestId = sessionData.getId();

        if( !sessionData.headersSent() ) {
            LOGGER_DEBUG( "Session closed without being handled, requestId: " + requestId );
            m_api.emit( Resources::Metrics::eventRequestUnhandled, Resources::EndpointHandlers::Cameras, session );
        }
        else {
            LOGGER_INFO_RAW( "Request finished: requestId: " + requestId );
        }
    }

    m_api.finishSession( params.m_wsi );
    m_api.emit( Resources::Metrics::eventParallelRequests, Resources::EndpointHandlers::Main, session );
    return httpDummy( params );
}

int MainHttpEndpoint::onTimer( CallbackParams params, BufferPosition bufPos ) {
    SessionPtr session = m_api.findSession( params.m_wsi );
    if( !session || !session->getData() )
        return -1;
    auto& sessionData = *session->getData();
    if( session->sessionType() == Session::SessionType::EVENT_STREAM )
        return httpDummy( params );

    LOGGER_TRACE_PRINT( "httpCallback / timer: wsi: %p", params.m_wsi );

    time_t now = time( nullptr );
    bool isTimedOut = now - sessionData.created() >= ApiBridge::requestDurationLimit() / 1000;

    if( isTimedOut ) {
        LOGGER_TRACE_PRINT( "httpCallback / request timeout reached: wsi: %p", params.m_wsi );
        m_api.emit( Resources::Metrics::eventRequestTimeOut, Resources::EndpointHandlers::Main, session );

        if( !sessionData.headersSent() ) {
            const bool closeSession = true;
            return ApiBridge::writeHttpStatusHeaders(
                sessionData, HTTP_STATUS_REQUEST_TIMEOUT, "", bufPos.m_start, bufPos.m_p, bufPos.m_end, closeSession );
        }
        else {
            if( lws_http_transaction_completed( params.m_wsi ) )
                return -1;
        }
    }

    return httpDummy( params );
}

}  // namespace HttpBridge
}  // namespace HubPlatform