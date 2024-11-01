#include "reflect_endpoint.h"
#include "http_bridge/session.h"
#include "http_bridge/session_manager.h"
#include "http_bridge/resources.h"
#include "http_bridge/ws_api_bridge.h"
#include "http_bridge/utils.h"
#include "common_utils/cpp14.h"
#include "database_client/dbid.h"
#include "tiny/tiny_consolecolors.h"
#include "log_utils/ezlo_log.h"

namespace HubPlatform {
namespace Tests {

using namespace HubPlatform::HttpBridge;

ReflectEndpoint::ReflectEndpoint( HttpBridge::ISessionManager& sessions )
: EndpointHandlers( { { LWS_CALLBACK_HTTP, &ReflectEndpoint::onHttp },
                      { LWS_CALLBACK_HTTP_WRITEABLE, &ReflectEndpoint::onHttpWriteable },
                      { LWS_CALLBACK_HTTP_BODY, &ReflectEndpoint::onHttpBody },
                      { LWS_CALLBACK_HTTP_BODY_COMPLETION, &ReflectEndpoint::onHttpBodyCompletion },
                      { LWS_CALLBACK_CLOSED_HTTP, &ReflectEndpoint::onClosedHttp },
                      { LWS_CALLBACK_TIMER, &ReflectEndpoint::onTimer } } )
, m_sessions( sessions ) {}

HttpBridge::EndpointPtr ReflectEndpoint::makeEndpoint( HttpBridge::ISessionManager& sessions ) {
    return std::make_unique< ReflectEndpoint >( sessions );
}

int ReflectEndpoint::endpointCallback( struct lws* wsi,
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

int ReflectEndpoint::httpDummy( HttpBridge::CallbackParams params ) {
    return HttpBridge::httpDummy( params );
}

int ReflectEndpoint::onHttp( HttpBridge::CallbackParams params, HttpBridge::BufferPosition bufPos ) {
    auto* wsi = params.m_wsi;
    char* url = (char*) params.m_in;
    auto *start = bufPos.m_start, *&p = bufPos.m_p, *end = bufPos.m_end;

    SessionPtr session = m_sessions.startSession( wsi, Resources::sessionTypeHttp );
    if( !session.get() || !session->getData() )
        return -1;
    SessionData& sessionData = *session->getData();

    if( url ) {
        std::string urlString{ "/reflect" };
        urlString += url;
        sessionData.getPath().assign( urlString );
    }

    session->extractHeaders( wsi );
    auto debugInfo = cc::debug( "ReflectEndpoint::onHttp: Url:" ) + url;
    auto peersDebugInfo = sessionData.peerDebugInfo();

    debugInfo += " ,";
    debugInfo += cc::j( peersDebugInfo );

    LOGGER_DEBUG_RAW( debugInfo );

    std::array< lws_token_indexes, 3 > tokensWithExpectHeader = {
        WSI_TOKEN_POST_URI, WSI_TOKEN_PATCH_URI, WSI_TOKEN_PUT_URI
    };

    bool isMethodWithExpect =
        std::find( tokensWithExpectHeader.begin(), tokensWithExpectHeader.end(), session->method ) !=
        tokensWithExpectHeader.end();

    if( isMethodWithExpect ) {
        // wait for POST body completion, then call ApiBridge::onMessage().
        if( sessionData.hasHeader( "expect:", "100-continue" ) ) {
            ApiBridge::writeHttpExpect100Response( wsi );
        }
        // Exit here to prevent onWritable callback call
        return 0;
    }
    else {
        auto sessionCount = m_sessions.sessionCount();
        if( sessionCount > ApiBridge::parallelSessionsLimit() ) {
            const bool closeSession = true;
            ApiBridge::errorParallelSessionLimitReached( sessionData, sessionCount, start, p, end, closeSession );
            return 1;
        }

        auto delayedVal = sessionData.getCommand().findParam( "__delayed" );
        if( !delayedVal.empty() )
            sessionData.delayed( stoi( delayedVal ) );

        auto method = HttpBridge::Utils::extractMethod( wsi );
        // All data came in, call onMessage()
        emitMessage( method, session );

        // use timer to limit request duration
        auto microSeconds = ApiBridge::requestDurationLimit() * 1000;
        lws_set_timer_usecs( wsi, microSeconds );
    }

    return 0;
}

int finalizeOptionsRequest( HttpBridge::CallbackParams& params,
                            HttpBridge::BufferPosition& bufPos,
                            const SessionPtr& session ) {
    if( session == nullptr )
        return 1;
    const bool isRequestOptions = session->method == WSI_TOKEN_OPTIONS_URI;
    if( isRequestOptions ) {
        auto wsi = session->getData()->getWsi();
        auto *start = bufPos.m_start, *&p = bufPos.m_p, *end = bufPos.m_end;
        int n = LWS_WRITE_HTTP;

        if( lws_add_http_common_headers( wsi,
                                         HTTP_STATUS_OK,
                                         "text/plain",
                                         LWS_ILLEGAL_HTTP_CONTENT_LEN, /* no content len */
                                         &p,
                                         end ) )
            return 1;
        if( lws_finalize_write_http_header( wsi, start, &p, end ) )
            return 1;

        session->getData()->headersSent( true );
        session->getData()->setSerializedAnswer( "GET, POST, PUT, OPTIONS, DELETE" );
        return -1;
    }
    return 0;
}

namespace QueryParams {
const char* const MakeRedirect = "make_redirect";
const char* const MultiplyHeader = "multiply_header";
const char* const RepeatCount = "repeat_count";
}  // namespace QueryParams

int ReflectEndpoint::onHttpWriteable( HttpBridge::CallbackParams params, HttpBridge::BufferPosition bufPos ) {
    auto* wsi = params.m_wsi;
    auto *start = bufPos.m_start, *&p = bufPos.m_p, *end = bufPos.m_end;
    int n = LWS_WRITE_HTTP;

    SessionPtr session = m_sessions.findSession( wsi );
    if( !session || !session->getData() )
        return -1;
    auto& sessionData = *session->getData();
    HttpBridgeCommand& command = sessionData.getCommand();

    const bool isRequestOptions = session->method == WSI_TOKEN_OPTIONS_URI;
    if( sessionData.hasSerializedAnswer() || isRequestOptions ) {
        if( !sessionData.headersSent() ) {
            int result = finalizeOptionsRequest( params, bufPos, session );
            if( result == -1 ) {
                lws_callback_on_writable( wsi );
                return 0;
            }
            else if( result >= 1 ) {
                return result;
            }
        }

        // Send headers here
        if( !sessionData.isLoggedIn() ) {
            LOGGER_TRACE_RAW( cc::trace( "Wasn't authorized; wsi: " ) + cc::num016( (uint64_t) wsi ) );
            const bool closeSession = true;
            return ApiBridge::writeHttpStatusHeaders(
                sessionData, HTTP_STATUS_UNAUTHORIZED, "", start, p, end, closeSession );
        }

        if( !sessionData.headersSent() ) {
            // handle redirect
            std::string location;
            std::string makeRedirect = command.findParam( QueryParams::MakeRedirect );
            if( !makeRedirect.empty() ) {
                int redirectCounter = std::atoi( makeRedirect.c_str() );
                location = makeRedirectLocation( command.params(), sessionData.getPath(), redirectCounter );
                if( redirectCounter >= 1 ) {
                    return httpRedirect( params, bufPos, location, HTTP_STATUS_MOVED_PERMANENTLY );
                }
            }
            if( lws_add_http_common_headers( wsi,
                                             HTTP_STATUS_OK,
                                             "application/json",
                                             LWS_ILLEGAL_HTTP_CONTENT_LEN, /* no content len */
                                             &p,
                                             end ) )
                return 1;

            // handle multiplyHeaders
            std::string multiplyHeadersUriParam = sessionData.getCommand().findParam( QueryParams::MultiplyHeader );
            std::string repeatCountUriParam = sessionData.getCommand().findParam( QueryParams::RepeatCount );
            if( !multiplyHeadersUriParam.empty() && !repeatCountUriParam.empty() ) {
                int repeatCount = std::stoi( repeatCountUriParam );

                HttpBridge::BufferPosition bp( start, p, end );
                appendMultipleHeaders( params, bp, multiplyHeadersUriParam, repeatCount );
            }
            if( lws_finalize_write_http_header( wsi, start, &p, end ) )
                return 1;
            sessionData.headersSent( true );
            lws_callback_on_writable( wsi );
            return 0;
        }
    }

    // send body here
    /* write using portions of a size defined by lws */
    long toWriteLimit = end - p - 1;
    std::string portion = sessionData.getSerializedAnswer( toWriteLimit );
    size_t size = portion.size();

    LOGGER_TRACE_PRINT( "httpCallback / http_writeable: toWriteLimit: %u/%d, wsi: %p", size, (int) toWriteLimit, wsi );

    if( size == 0 || (int) size < toWriteLimit )
        n = LWS_WRITE_HTTP_FINAL;

    p += lws_snprintf( (char*) p, end - p, "%s", portion.c_str() );

    auto toWrite = lws_ptr_diff( p, start );
    auto bytesWritten = lws_write( wsi, (uint8_t*) start, toWrite, static_cast< lws_write_protocol >( n ) );
    sessionData.addBytesWritten( bytesWritten );

    if( bytesWritten != toWrite )
        return 1;

    if( n == LWS_WRITE_HTTP_FINAL ) {
        if( lws_http_transaction_completed( wsi ) )
            return -1;
    }
    else if( !sessionData.isPending() )
        lws_callback_on_writable( wsi );

    return 0;
}

int ReflectEndpoint::onHttpBody( HttpBridge::CallbackParams params, HttpBridge::BufferPosition bufPos ) {
    LOGGER_TRACE( "" );
    auto* wsi = params.m_wsi;
    char* data = (char*) params.m_in;
    const auto len = params.m_len;
    if( data && len > 0 ) {
        SessionPtr session = m_sessions.findSession( wsi );
        if( !session || !session->getData() )
            return -1;

        auto& buffer = session->getData()->getPostData();
        buffer.insert( buffer.end(), data, data + len );
        LOGGER_TRACE_PRINT( "reflect / http_body: %u", len );
    }
    return httpDummy( params );
}

int ReflectEndpoint::onHttpBodyCompletion( HttpBridge::CallbackParams params, HttpBridge::BufferPosition bufPos ) {
    LOGGER_TRACE( "" );
    auto* wsi = params.m_wsi;
    char* data = (char*) params.m_in;
    const auto len = params.m_len;
    auto *start = bufPos.m_start, *&p = bufPos.m_p, *end = bufPos.m_end;

    SessionPtr session = m_sessions.findSession( wsi );
    if( !session || !session->getData() )
        return -1;

    auto& sessionData = *session->getData();
    auto& buffer = sessionData.getPostData();
    if( data && len > 0 ) {
        buffer.insert( buffer.end(), data, data + len );
        LOGGER_TRACE_PRINT( "reflect / http_body_completion: %u, wsi: %p", len, wsi );
    }
    LOGGER_TRACE_PRINT( "reflect / http_body_completion: Total body size: %u, wsi: %p", buffer.size(), wsi );

    if( !sessionData.headersSent() ) {
        auto sessionCount = m_sessions.sessionCount();
        if( sessionCount > ApiBridge::parallelSessionsLimit() ) {
            const bool closeSession = true;
            ApiBridge::errorParallelSessionLimitReached( sessionData, sessionCount, start, p, end, closeSession );
            return 1;
        }
    }

    auto method = HttpBridge::Utils::extractMethod( wsi );
    emitMessage( method, session );

    // use timer to limit request duration
    auto microSeconds = ApiBridge::requestDurationLimit() * 1000;
    lws_set_timer_usecs( wsi, microSeconds );

    lws_callback_on_writable( wsi );

    return 0;
}

int ReflectEndpoint::onTimer( HttpBridge::CallbackParams params, HttpBridge::BufferPosition bufPos ) {
    return httpDummy( params );
}

int ReflectEndpoint::onClosedHttp( HttpBridge::CallbackParams params, HttpBridge::BufferPosition bufPos ) {
    LOGGER_TRACE_PRINT( "reflect / closed_http: wsi: %p", params.m_wsi );
    m_sessions.finishSession( params.m_wsi );

    return httpDummy( params );
}

void ReflectEndpoint::emitMessage( const std::string& httpMethod, HttpBridge::SessionPtr session ) {
    auto sessionData = session->getData();
    auto postData = sessionData->getPostData();

    std::string postDataStr( postData.data(), postData.size() );
    LOGGER_TRACE( httpMethod + ", body.size:" + cc::num10( postDataStr.size() ) );

    auto t = time( nullptr );
    Json result = Json::object();
    Json headers = Json::object();

    for( const auto& it : sessionData->knownHeaders() ) {
        headers[ it.first ] = it.second;
    }
    result[ "headers" ] = headers;

    char timeBuf[ 64 ];
    strftime( timeBuf, sizeof( timeBuf ), "%T", localtime( &t ) );
    result[ "now" ] = timeBuf;
    result[ "method" ] = httpMethod;
    if( httpMethod == "POST" || httpMethod == "PUT" || httpMethod == "PATCH" ) {
        result[ "post_body_size" ] = postData.size();
        result[ "post_body" ] = std::string{ postData.data(), postData.size() };
    }

    std::string dump = result.dump();
    if( sessionData->delayed() > 0 ) {
        sessionData->isPending( true );
        auto sender = [dump, session, sessionData]() {
            std::this_thread::sleep_for( std::chrono::milliseconds( sessionData->delayed() ) );
            sessionData->setSerializedAnswer( dump );
            sessionData->isPending( false );
            sessionData->isLoggedIn( true );
            sessionData->notify();
        };
        std::thread( std::move( sender ) ).detach();
    }
    else {
        sessionData->setSerializedAnswer( dump );
        sessionData->isPending( false );
        sessionData->isLoggedIn( true );
        sessionData->notify();
    }
}

void ReflectEndpoint::appendMultipleHeaders( HttpBridge::CallbackParams params,
                                             HttpBridge::BufferPosition& bufPos,
                                             std::string headerName,
                                             int repeatCount ) {
    if( headerName.empty() || repeatCount < 1 )
        return;
    LOGGER_DEBUG( "Appending multiple headers to response: " + headerName + ", count: " + cc::num10( repeatCount ) );

    for( int i = repeatCount; --i >= 0; ) {
        if( headerName.find_last_of( ':' ) != headerName.size() - 1 ) {  // should be like: "headerName:"
            headerName += ':';
        }
        std::string generatedValue = db::DbId::genOID();

        LOGGER_DEBUG( "Adding a header: " + headerName + ", value: " = generatedValue );
        lws_add_http_header_by_name( params.m_wsi,
                                     reinterpret_cast< const unsigned char* >( headerName.data() ),
                                     reinterpret_cast< const unsigned char* >( generatedValue.data() ),
                                     generatedValue.size(),
                                     &bufPos.m_p,
                                     bufPos.m_end );
    }
}

int ReflectEndpoint::httpRedirect( CallbackParams params,
                                   BufferPosition& bufPos,
                                   const std::string& location,
                                   int redirectCode ) {
    auto wsi = params.m_wsi;

    LOGGER_TRACE( cc::trace( "Making a redirect to " ) + location );
    if( lws_http_redirect(
            wsi, redirectCode, (const unsigned char*) location.data(), location.size(), &bufPos.m_p, bufPos.m_end ) <
        0 )
        return -1;
    return lws_http_transaction_completed( wsi );
}

std::string ReflectEndpoint::makeRedirectLocation( const HttpBridgeCommand::Params& params,
                                                   const std::string& urlBase,
                                                   int redirectCounter ) {
    std::string location;
    if( redirectCounter <= 0 )
        return location;

    std::string redirectionParams;
    for( const auto& param : params ) {
        if( param.first == QueryParams::MakeRedirect ) {
            if( redirectCounter > 1 ) {
                redirectionParams.append( "&" );
                redirectionParams.append( param.first + "=" + std::to_string( redirectCounter - 1 ) );
            }
        }
        else {
            if( !redirectionParams.empty() )
                redirectionParams.append( "&" );
            redirectionParams.append( param.first + "=" + param.second );
        }
    }
    location = urlBase;
    if( !redirectionParams.empty() ) {
        location += "?" + redirectionParams;
    }

    return location;
}

}  // namespace Tests
}  // namespace HubPlatform