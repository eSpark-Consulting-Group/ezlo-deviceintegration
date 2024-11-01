#include "uid/extensions/http_links/http_links_http_extension.h"
#include "uid/extensions/http_links/events.h"
#include "uid/extensions/http_links/errors.h"
#include "uid/extensions/http_links/post_user_data.h"
#include "uid/extensions/http_links/resources.h"
#include "uid/extensions/ui_http_utils.h"
#include "log_utils/ezlo_log.h"
#include "common_utils/cpp14.h"
#include "tiny/tiny_consolecolors.h"
#include "base_utils/ezlo_common.h"
#include "http_bridge/utils.h"
#include "http_bridge/resources.h"

namespace HubPlatform {
namespace HttpLinks {

const int IGNORE_RESULT = -1;       //!< UiExtensions will not report this result to libwebsockets
const int CLOSE_CONNECTION = 1;     //!< Return this to close the connection
const int CONTINUE_CONNECTION = 0;  //!< return this to continue the connection event flow.

namespace {
void fetchClientIp( lws* wsi, PostUserData& userData ) {
    const int IP_SIZE = 64;
    char clientName[ IP_SIZE ];
    char clientIp[ IP_SIZE ];
    ::memset( clientName, 0, sizeof( clientName ) );
    ::memset( clientIp, 0, sizeof( clientIp ) );

    int socketFd = ::lws_get_socket_fd( wsi );
    ::lws_get_peer_addresses( wsi, socketFd, clientName, sizeof( clientName ) - 1, clientIp, sizeof( clientIp ) - 1 );
    std::string ip{ clientIp };
    std::string name{ clientName };

    userData.m_clientIp = ip;
}

uint64_t timeSinceEpochMilliseconds() {
    using namespace std::chrono;
    return duration_cast< milliseconds >( system_clock::now().time_since_epoch() ).count();
}
}  // namespace

HttpLinksHttpExtension::HttpLinksHttpExtension( HttpLinksManager& manager )
: m_manager( manager ) {}

/** Handles the route: /v1/links/plugins/plugin_id/link_id */
int HttpLinksHttpExtension::onHttp( struct lws* wsi, HttpSession& session ) {
    const char* logPrefix = "HttpLinksHttpExtension::onHttp: ";
    Log::info( cc::str( logPrefix ) + session.method + " " + session.url );
    // TODO: etag, random

    using namespace Uid::Extensions::Utils;
    using namespace Resources;
    static std::string httpLinksPrefix = "/plugins/";
    if( strncmp( session.url.c_str(), httpLinksPrefix.c_str(), httpLinksPrefix.size() ) != 0 ) {
        return writeError( wsi, HttpCodes::NotFound, "http_links addon does not provide specified functionality" );
    }

    auto tokenList = ezlo::common::APIs::split_string( session.url.c_str() + httpLinksPrefix.size(), "/" );

    if( tokenList.empty() || tokenList.size() < 2 ) {
        // error: invalid parameters; plugin_id, link_id expected
        LOGGER_ERROR_RAW( cc::debug( logPrefix ) + cc::error( "Insufficient parameters: 2 expected, given " ) +
                          cc::num10( tokenList.size() ) );
        return writeError( wsi, HttpCodes::BadRequest, "Bad Request: insufficient parameters: 2 expected" );
    }
    else {
        auto pluginId = tokenList.front();
        tokenList.pop_front();
        auto linkId = tokenList.front();

        Json linkInfo;
        try {
            namespace Fields = Resources::HttpLinkFields;
            linkInfo = m_manager.getLinkInfoRaw( linkId );
            if( linkInfo.empty() )
                throw HttpLinksError( "Http Link does not exist" );
            if( json_get_str( linkInfo, Fields::pluginId ) != pluginId )
                throw HttpLinksError( "Can fetch only own http links" );
            if( json_get_int( linkInfo, Resources::HttpLinkFields::deleted, 0 ) > 0 )
                throw HttpLinksError( "Http Link is deleted" );
        }
        catch( const HttpLinksError& ex ) {
            LOGGER_ERROR( cc::error( "getLinkInfo exception: " ) + cc::note( ex.what() ) );
            return writeError( wsi, HttpCodes::NotFound, std::string{ "getLinkInfo: " } + ex.what() );
        }

        if( linkInfo.empty() ) {
            LOGGER_DEBUG_RAW( cc::debug( std::string{ logPrefix } + "Plugin http link does not exist; " ) +
                              cc::note( "(plugin_id, link_id) = (" ) + pluginId + ", " + linkId + ")" );
            return writeError( wsi, HttpCodes::NotFound, "Plugin http link does not exist" );
        }
        else {
            LOGGER_DEBUG_RAW( cc::debug( std::string{ logPrefix } + "Plugin http link: " ) +
                              cc::note( "(plugin_id, link_id) = (" ) + pluginId + ", " + linkId + ")" );
        }

        bool waitForData = false;
        {
            auto userData = std::make_unique< PostUserData >();
            userData->linkId( linkId );
            userData->pluginId( pluginId );
            userData->m_uriParams = HttpBridge::Utils::extractUriParams( wsi );

            fetchClientIp( wsi, *userData );
            userData->m_created = timeSinceEpochMilliseconds();
            waitForData = json_get_bool( linkInfo, "answer", false );

            userData->waitForData( waitForData );
            userData->m_timeout = json_get_int( linkInfo, "timeout", Resources::Limits::RESPONSE_TIMEOUT_DEFAULT );
            if( userData->m_timeout <= 0 ) {
                userData->m_timeout = Resources::Limits::RESPONSE_TIMEOUT_DEFAULT;
            }
            else if( userData->m_timeout > Resources::Limits::RESPONSE_TIMEOUT_LIMIT ) {
                userData->m_timeout = Resources::Limits::RESPONSE_TIMEOUT_LIMIT;
            }
            // use timer to limit request duration, microseconds
            lws_set_timer_usecs( wsi, userData->m_timeout * 1000 );
            lws_set_timeout( wsi, PENDING_TIMEOUT_HTTP_KEEPALIVE_IDLE, 20 );

            session.userData = std::move( userData );
        }

        if( session.method == HttpBridge::Resources::httpMethodGet ||
            session.method == HttpBridge::Resources::httpMethodPost ) {
            emitConnectionCreated( wsi, session );
        }
        else {
            LOGGER_DEBUG( cc::debug( "Method not supported: " ) + cc::note( session.method ) );
            return writeError( wsi, HttpCodes::MethodNotAllowed, "Method not allowed" );
        }

        if( session.method == HttpBridge::Resources::httpMethodGet ) {
            // move emit to http_writable
            lws_callback_on_writable( wsi );
            return CONTINUE_CONNECTION;
        }
        else if( session.method == HttpBridge::Resources::httpMethodPost ) {
            if( session.hasHeader( "expect", "100-continue" ) ) {
                // Write 100 continue response
                uint8_t buf[ LWS_PRE + LWS_RECOMMENDED_MIN_HEADER_SPACE ];
                uint8_t *start = &buf[ LWS_PRE ], *p = start, *end = &buf[ sizeof( buf ) - 1 ];
                lws_add_http_header_status( wsi, HttpCodes::Continue, &p, end );
                lws_finalize_write_http_header( wsi, start, &p, end );
            }
        }
        // POST: continue in onHttpBody
    }

    return CONTINUE_CONNECTION;
}

int HttpLinksHttpExtension::onHttpBody( struct lws* wsi, HttpSession& session, const char* data, size_t size ) {
    if( !session.userData ) {
        LOGGER_DEBUG( "HttpLinks: no user data" );
        return CLOSE_CONNECTION;
    }
    auto userDataPtr = sessionUserData( session );
    if( !userDataPtr ) {
        LOGGER_DEBUG( "HttpLinks: session user data type mismatch" );
        return CLOSE_CONNECTION;
    }
    auto bodySizeReceived = userDataPtr->m_body.size() + userDataPtr->m_bodyStreamedSize;
    if( userDataPtr->m_bodyPortions++ == 0 ||
        ( userDataPtr->m_body.size() ) / 100000 != ( userDataPtr->m_body.size() + size ) / 100000 ) {
        LOGGER_DEBUG_RAW( cc::debug( "HttpLinksHttpExtension::onHttpBody, N" +
                                     cc::num10( userDataPtr->m_bodyPortions ) + ", size=" + cc::num10( size ) +
                                     ", total=" + cc::num10( bodySizeReceived ) ) );
    }

    const auto maxPortion = Resources::Limits::MAX_DATA_RECEIVED_PORTION;
    auto toWrite = size;
    if( maxPortion - userDataPtr->m_body.size() < size )
        toWrite = maxPortion - userDataPtr->m_body.size();
    if( toWrite > 0 ) {
        userDataPtr->m_body.insert( userDataPtr->m_body.end(), data, data + toWrite );
    }

    if( userDataPtr->m_body.size() >= maxPortion ) {
        const bool notLast = false;
        LOGGER_TRACE( cc::trace( "emit next portion: " ) + cc::num10( userDataPtr->m_body.size() ) + "/" +
                      cc::num10( userDataPtr->m_bodyStreamedSize + userDataPtr->m_body.size() ) );
        emitDataReceived( wsi, session, notLast );
    }

    if( toWrite < size ) {
        userDataPtr->m_body.insert( userDataPtr->m_body.end(), data + toWrite, data + size );
    }

    return CONTINUE_CONNECTION;
}

int HttpLinksHttpExtension::onHttpBodyCompletion( struct lws* wsi, HttpSession& session ) {
    LOGGER_DEBUG( "HttpLinks" );
    if( !session.userData ) {
        LOGGER_DEBUG( "HttpLinks: no user data" );
        return CLOSE_CONNECTION;
    }
    auto userDataPtr = sessionUserData( session );
    if( !userDataPtr ) {
        LOGGER_DEBUG( "HttpLinks: session user data type mismatch" );
        return CLOSE_CONNECTION;
    }
    size_t totalSize = userDataPtr->m_bodyStreamedSize + userDataPtr->m_body.size();
    if( totalSize > 64 * 1024 ) {
        LOGGER_DEBUG_RAW( cc::debug( "Got all data of a POST request, bytes: " ) + cc::num10( totalSize ) );
    }

    emitDataReceived( wsi, session, true );
    lws_set_timeout( wsi, PENDING_TIMEOUT_HTTP_KEEPALIVE_IDLE, 30 );

    if( userDataPtr->waitForData() ) {
        emitDataSend( wsi, session );
    }
    lws_callback_on_writable( wsi );

    return CONTINUE_CONNECTION;
}

int HttpLinksHttpExtension::onHttpWriteable( struct lws* wsi, HttpSession& session ) {
    auto* userDataPtr = sessionUserData( session );
    if( !userDataPtr ) {
        LOGGER_DEBUG( "HttpLinks: session user data type mismatch" );
        return CLOSE_CONNECTION;
    }
    auto& userData = *userDataPtr;

    if( !userData.dataReceivedSent() ) {
        emitDataReceived( wsi, session, true );
        if( userData.waitForData() )
            emitDataSend( wsi, session );
        lws_callback_on_writable( wsi );
        return CONTINUE_CONNECTION;
    }

    if( m_manager.connectionForceClosed( userData.connectionId() ) ) {
        LOGGER_INFO( cc::info( "Connection closed from outside: " ) + cc::note( userData.connectionId() ) );
        // was decided to close the connection from outside
        userData.closedReason( ConnectionClosedReason::CLOSED_BY_CLIENT );
        if( !session.responseHeadersWritten ) {
            using namespace Uid::Extensions::Utils;
            using namespace Resources;
            emitConnectionClosed( wsi, session, __LINE__ );
            session.responseHeadersWritten = true;
            return writeError( wsi, HttpCodes::ClientClosedRequest, "Client Closed Request" );
        }
        emitConnectionClosed( wsi, session, __LINE__ );
        return CLOSE_CONNECTION;
    }

    uint8_t buf[ LWS_PRE + LWS_RECOMMENDED_MIN_HEADER_SPACE ];
    uint8_t *start = &buf[ LWS_PRE ], *p = start, *end = &buf[ sizeof( buf ) - 1 ];

    using namespace Resources;
    if( !session.responseHeadersWritten ) {
        if( userData.waitForData() && !m_manager.isFirst( userData.connectionId() ) ) {
            return CONTINUE_CONNECTION;
        }

        ExitCodes exitCodes;
        if( m_manager.getExitCodes( userData.connectionId(), exitCodes ) ) {
            auto code = exitCodes.code != 0 ? exitCodes.code : HttpCodes::Ok;
            lws_filepos_t contentLength = userData.m_response.size();
            if( exitCodes.code >= 300 )
                contentLength = 0;

            if( lws_add_http_common_headers( wsi, code, "", LWS_ILLEGAL_HTTP_CONTENT_LEN, &p, end ) ) {
                return CLOSE_CONNECTION;
            }
            if( !exitCodes.headers.empty() ) {
                std::string preparedHeader;

                for( const auto& header : exitCodes.headers ) {
                    std::string name = header.first;
                    if( name.find_last_of( ':' ) != name.size() - 1 ) {  // should be like: "headerName:"
                        name += ':';
                    }
                    lws_add_http_header_by_name( wsi,
                                                 reinterpret_cast< const unsigned char* >( name.data() ),
                                                 reinterpret_cast< const unsigned char* >( header.second.data() ),
                                                 header.second.size(),
                                                 &p,
                                                 end );
                }
            }
            else {
                std::string contentLengthValue = std::to_string( userData.m_response.size() );
                lws_add_http_header_by_name( wsi,
                                             reinterpret_cast< const unsigned char* >( "content-length:" ),
                                             reinterpret_cast< const unsigned char* >( contentLengthValue.data() ),
                                             contentLengthValue.size(),
                                             &p,
                                             end );
            }
        }
        else {
            if( lws_add_http_common_headers(
                    wsi, HttpCodes::Ok, "application/json", userData.m_response.size(), &p, end ) )
                return CLOSE_CONNECTION;
        }

        session.responseHeadersWritten = true;

        if( exitCodes.code >= 300 ) {
            lws_finalize_http_header( wsi, &p, end );
            lws_write( wsi, start, lws_ptr_diff( p, start ), LWS_WRITE_HTTP_HEADERS );

            userData.closedReason( ConnectionClosedReason::OK );
            emitConnectionClosed( wsi, session, __LINE__ );
            if( lws_http_transaction_completed( wsi ) )
                return CLOSE_CONNECTION;
        }
        else {
            if( lws_finalize_write_http_header( wsi, start, &p, end ) )
                return CLOSE_CONNECTION;
            lws_callback_on_writable( wsi );
        }

        return CONTINUE_CONNECTION;
    }

    /** @returns true if there is one more portion extracted OR connection.last() == false */
    auto fetchNextPortion = [this]( PostUserData& userData ) -> bool {
        if( !userData.waitForData() )
            return false;

        bool result = m_manager.fetchNextPortion( userData.connectionId(), userData.m_response );
        return result;
    };

    auto toWriteLimit = end - p - 1;
    auto bytesLeft = userData.leftInResponse();

    if( userData.leftInResponse() == 0 && userData.waitForData() ) {
        userData.clearResponse();

        if( fetchNextPortion( userData ) ) {
            if( userData.leftInResponse() > 0 ) {
                lws_callback_on_writable( wsi );

                // extends the time of life for response and doesn't interfere with lws_set_timer_usecs() which limits
                // the whole request lifetime.
                lws_set_timeout( wsi, PENDING_TIMEOUT_HTTP_KEEPALIVE_IDLE, 30 );
            }
            else {
                if( m_manager.dataSendEventRequested( userData.connectionId() ) ) {
                    m_manager.dataSendEventRequested( userData.connectionId(), false );
                    emitDataSend( wsi, session );
                }
            }
            return CONTINUE_CONNECTION;
        }
    }

    auto ptr = userData.m_response.data() + userData.m_position;
    auto size = 0;

    if( toWriteLimit <= bytesLeft ) {
        size = toWriteLimit;
    }
    else {
        size = bytesLeft;
    }
    std::string portion;
    if( size > 0 )
        portion.assign( ptr, size );
    userData.moveResponsePosition( size );

    int n = LWS_WRITE_HTTP;
    if( !userData.waitForData() ) {
        if( size == 0 ) {
            n = LWS_WRITE_HTTP_FINAL;
        }
    }
    else if( m_manager.isLast( userData.connectionId() ) && size == 0 && userData.leftInResponse() == 0 ) {
        n = LWS_WRITE_HTTP_FINAL;
    }

    p += lws_snprintf( (char*) p, end - p, "%s", portion.c_str() );
    auto toWrite = lws_ptr_diff( p, start );
    errno = 0;
    auto bytesWritten =
        lws_write( wsi, (uint8_t*) start, toWrite, static_cast< lws_write_protocol >( n | LWS_WRITE_BINARY ) );
    if( bytesWritten < 0 ) {
        if( errno == ECONNRESET ) {
            LOGGER_ERROR( cc::error( strerror( errno ) ) );
            userData.closedReason( ConnectionClosedReason::CLOSED_BY_CLIENT );
            emitConnectionClosed( wsi, session, __LINE__ );
            return CLOSE_CONNECTION;
        }
        else if( errno != 0 ) {
            LOGGER_ERROR( cc::error( "could not write to connection, error: (" ) + cc::num10( errno ) + ") " +
                          cc::error( strerror( errno ) ) );
            userData.closedReason( ConnectionClosedReason::UNKNOWN, errno != 0 ? strerror( errno ) : nullptr );
            emitConnectionClosed( wsi, session, __LINE__ );
            return CLOSE_CONNECTION;
        }
    }
    else {
        if( bytesWritten != toWrite ) {
            LOGGER_ERROR( cc::error( "write error: " ) + cc::num10( bytesWritten ) + ", " + cc::num10( toWrite ) +
                          cc::error( ", error: (" ) + cc::num10( errno ) + ") " + std::string{ strerror( errno ) } );
            userData.closedReason( ConnectionClosedReason::CLOSED_BY_SERVER );
            emitConnectionClosed( wsi, session, __LINE__ );
            return CLOSE_CONNECTION;
        }
    }

    if( n == LWS_WRITE_HTTP_FINAL ) {
        userData.closedReason( ConnectionClosedReason::OK );
        emitConnectionClosed( wsi, session, __LINE__ );
        if( lws_http_transaction_completed( wsi ) )
            return CLOSE_CONNECTION;
    }
    else {
        lws_callback_on_writable( wsi );
    }

    return CONTINUE_CONNECTION;
}

/** @brief Check if default or custom timeout is reached */
int HttpLinksHttpExtension::onTimer( struct lws* wsi, HttpSession& session ) {
    if( !session.userData ) {
        return CLOSE_CONNECTION;
    }

    auto userData = sessionUserData( session );
    if( !userData ) {
        return CLOSE_CONNECTION;
    }

    const auto now = timeSinceEpochMilliseconds();
    bool isTimedOut = ( now - userData->m_created ) >= userData->m_timeout;

    LOGGER_DEBUG( cc::debug( "Timeout value: " ) + cc::num10( userData->m_timeout ) + cc::debug( ", time passed: " ) +
                  cc::num10( now - userData->m_created ) + cc::debug( ", connection id: " ) +
                  cc::note( userData->connectionId() ) );

    if( isTimedOut ) {
        userData->closedReason( ConnectionClosedReason::TIMEOUT_IS_REACHED );

        if( !session.responseHeadersWritten ) {
            using namespace Uid::Extensions::Utils;
            session.responseHeadersWritten = true;
            writeError( wsi, HTTP_STATUS_REQUEST_TIMEOUT, "Request Timeout" );
        }
        else {
            lws_http_transaction_completed( wsi );
        }
        emitConnectionClosed( wsi, session, __LINE__ );
        return CLOSE_CONNECTION;
    }

    return CONTINUE_CONNECTION;
}

bool HttpLinksHttpExtension::authorizationRequired() const {
    return false;
}

int HttpLinksHttpExtension::onHttpClosed( struct lws* wsi, HttpSession& session ) {
    auto* userData = sessionUserData( session );
    if( !userData )
        return CLOSE_CONNECTION;
    LOGGER_TRACE( "connection_id: " + userData->connectionId() +
                  ", reason: " + cc::num10( toInt( userData->reason() ) ) );
    if( userData->reason() == ConnectionClosedReason::NONE )
        userData->closedReason( ConnectionClosedReason::CLOSED_BY_SERVER );
    time( &userData->m_finished );
    emitConnectionClosed( wsi, session, __LINE__ );

    return CONTINUE_CONNECTION;
}

PostUserData* HttpLinksHttpExtension::sessionUserData( const HttpSession& session ) {
    return dynamic_cast< PostUserData* >( session.userData.get() );
}

/** @brief Called when connection to a link is created, passes http call parameters to store.  */
void HttpLinksHttpExtension::emitConnectionCreated( lws* wsi, HttpSession& session ) {
    auto* userData = sessionUserData( session );
    if( !userData )
        return;

    ConnectionInfo::Headers headersArr;
    for( const auto& pair : session.headers ) {
        headersArr.push_back( pair );
    }

    ConnectionInfo info( userData->linkId(), headersArr, userData->m_uriParams );
    info.created( userData->m_created / 1000 );  // ms to sec
    info.clientIp( userData->m_clientIp );

    userData->connectionId( m_manager.createConnection( userData->linkId(), info, wsi ) );
}

/** @brief Called when connection to a link is closed by lws-server, passes http call parameters to store.  */
void HttpLinksHttpExtension::emitConnectionClosed( lws* /* wsi */, HttpSession& session, int line ) {
    auto* userData = sessionUserData( session );
    if( !userData )
        return;

    ConnectionInfo connectionInfo;
    m_manager.getConnectionInfo( userData->connectionId(), connectionInfo );

    if( !m_manager.markConnectionAsClosed( userData->connectionId() ) ) {
        LOGGER_TRACE( "Already closed" );
        return;
    }

    namespace Fields = HttpLinks::Resources::HttpLinkFields;
    // get even just deleted links
    Json linkUserData, linkInfo = m_manager.getLinkInfoRaw( userData->linkId() );
    if( !linkInfo.empty() )
        json_get( linkInfo, Fields::userData, linkUserData );

    auto reason = userData->reason();
    auto text = userData->reasonText();
    if( text.empty() )
        text = reasonText( reason );

    ConnectionClosedEvent event( userData->connectionId(), userData->linkId(), linkUserData, reason, text );
    m_manager.eventHandler().onHttpConnectionClosed( event );
}

void HttpLinksHttpExtension::emitDataReceived( lws* /* wsi */, HttpSession& session, bool last ) {
    auto* userData = sessionUserData( session );
    if( !userData )
        return;

    ConnectionInfo connectionInfo;
    if( !m_manager.getConnectionInfo( userData->connectionId(), connectionInfo ) ) {
        LOGGER_WARN( cc::warn( "HttpLinks connection could not be found: id=" ) +
                     cc::note( userData->connectionId() ) );
        return;
    }

    namespace Fields = HttpLinks::Resources::HttpLinkFields;
    Json linkInfo = m_manager.getLinkInfoRaw( userData->linkId() );
    Json linkUserData;
    if( json_get( linkInfo, Fields::userData, linkUserData ) ) {
    }

    DataReceivedEvent event( userData->connectionId(),
                             userData->linkId(),
                             std::move( linkUserData ),
                             DataReceivedEvent::Headers{ session.headers.begin(), session.headers.end() },
                             userData->m_uriParams,
                             std::string( userData->m_body.data(), userData->m_body.size() ),
                             last,
                             session.method );
    m_manager.eventHandler().onHttpDataReceived( event );
    userData->dataReceivedSent( true );
    if( !userData->m_body.empty() ) {
        userData->m_bodyStreamedSize += userData->m_body.size();
        userData->m_body.clear();
    }
}

/** @brief Send event http_data_send to a handler */
void HttpLinksHttpExtension::emitDataSend( lws* /* wsi */, HttpSession& session ) {
    auto* userData = sessionUserData( session );
    if( !userData )
        return;

    ConnectionInfo connectionInfo;
    if( !m_manager.getConnectionInfo( userData->connectionId(), connectionInfo ) ) {
        LOGGER_WARN( cc::warn( "HttpLinks connection could not be found: id=" ) +
                     cc::note( userData->connectionId() ) );
        return;
    }

    namespace Fields = HttpLinks::Resources::HttpLinkFields;
    Json linkInfo = m_manager.getLinkInfoRaw( userData->linkId() );
    Json linkUserData;
    json_get( linkInfo, Fields::userData, linkUserData );

    const int MAX_DATA_SEND = Resources::Limits::MAX_DATA_SEND_PORTION;
    DataSendEvent event( userData->connectionId(), userData->linkId(), std::move( linkUserData ), MAX_DATA_SEND );
    m_manager.eventHandler().onHttpDataSend( event );
}

bool HttpLinksHttpExtension::hasTimerHandler() const {
    return true;
}

}  // namespace HttpLinks
}  // namespace HubPlatform
