#include "http/http_multi_client.h"
#include "http/http_error.h"

namespace HubPlatform {
namespace CommonHttp {

HttpMultiClient::HttpMultiClient( HttpPluginConnections& impl, PluginConnectionQueue& connectionQueue )
: m_multi( curl_multi_init(), curl_multi_cleanup )
, m_connections( impl )
, m_connectionQueue( connectionQueue ) {}

HttpMultiClient::~HttpMultiClient() {
    reset();
}

void HttpMultiClient::reset() {
    m_multi.reset();
}

bool HttpMultiClient::addMultiHandle( Connection& connection ) {
    CURLMcode code = curl_multi_add_handle( m_multi.get(), connection.getCURL() );
    if( code != CURLM_OK ) {
        LOGGER_ERROR( cc::error( "Error while adding connection to multi-handle: " ) + connection.getId() +
                      cc::error( ", error: " ) + cc::attention( curl_multi_strerror( code ) ) );
        throw HttpError( "Error in curl_multi_add_handle : " + std::string( curl_multi_strerror( code ) ) );
    }
    connection.inMulti( true );
    return true;
}

bool HttpMultiClient::removeMultiHandle( Connection& connection ) {
    if( !connection.inMulti() )
        return false;
    CURLMcode code = curl_multi_remove_handle( m_multi.get(), connection.getCURL() );
    if( code != CURLM_OK ) {
        LOGGER_ERROR( cc::error( "Error while removing connection to multi-handle: " ) +
                      cc::note( connection.getId() ) + cc::error( ", error: " ) +
                      cc::attention( curl_multi_strerror( code ) ) );
        throw HttpError( "Error in curl_multi_remove_handle : " + std::string( curl_multi_strerror( code ) ) );
    }
    connection.inMulti( false );
    LOGGER_TRACE( cc::trace( "id: " ) + cc::note( connection.getId() ) );
    return true;
}

Reason HttpMultiClient::getReason( const CURLMsg* msg, Connection* connection ) {
    Reason reason = Reason::NONE;
    if( !msg )
        return reason;

    if( isSSLConnectionError( msg->data.result ) )
        return Reason::SSL_CONNECTION_ERROR;

    switch( msg->data.result ) {
    case CURLE_COULDNT_CONNECT:
        reason = Reason::CONNECTION_ERROR;
        break;
    case CURLE_COULDNT_RESOLVE_HOST:
        reason = Reason::DNS_NAME;
        break;
        // case ???:
        //    reason = Reason::CLOSED_BY_CLIENT; break;
        // case ???:
        //    reason = Reason::CLOSED_BY_SERVER; break;
    case CURLE_HTTP_RETURNED_ERROR:
        reason = Reason::CLOSED_BY_SERVER;
        break;
    case CURLE_OPERATION_TIMEDOUT:
        reason = Reason::RESPONSE_TIMEOUT_REACHED;
        break;
    case CURLE_TOO_MANY_REDIRECTS:
        reason = Reason::TOO_MANY_REDIRECTS;
        break;
    default:
        if( connection ) {
            auto httpStatus = connection->response().getHttpStatus();
            if( httpStatus >= 400 && httpStatus < 600 )
                reason = Reason::CLOSED_BY_SERVER;
        }
        if( Reason::NONE != reason )
            reason = Reason::UNKNOWN_ERROR;
    }
    return reason;
}

bool HttpMultiClient::isSSLConnectionError( CURLcode code ) {
    switch( code ) {
    // SSL connection errors:
    case CURLE_SSL_ENGINE_NOTFOUND:
    case CURLE_SSL_ENGINE_SETFAILED:
    case CURLE_SSL_CERTPROBLEM:
    case CURLE_SSL_CIPHER:
    case CURLE_SSL_CACERT:
    case CURLE_SSL_ENGINE_INITFAILED:
    case CURLE_SSL_CACERT_BADFILE:
    case CURLE_SSL_CRL_BADFILE:
    case CURLE_SSL_ISSUER_ERROR:
    case CURLE_SSL_PINNEDPUBKEYNOTMATCH:
    case CURLE_SSL_INVALIDCERTSTATUS:
    case CURLE_PEER_FAILED_VERIFICATION:  // SSL domain certificate check error
    case CURLE_SSL_CONNECT_ERROR:
        return true;
    default:
        return false;
    }
}

void HttpMultiClient::runMultiHandleThread( std::shared_ptr< std::promise< void > > readyToRun ) {
    int stillAlive = 0;
    int repeats = 0;
    bool shouldContinue = false;
    time_t wasStillAlive = time( nullptr );

    curl_multi_setopt( m_multi.get(), CURLMOPT_PIPELINING, CURLPIPE_MULTIPLEX );

    readyToRun->set_value();

    do {
        // add/remove new handles first
        clearQueue();

        auto* multi = m_multi.get();
        CURLMcode mc = curl_multi_perform( multi, &stillAlive );
        if( CURLM_OK == mc )
            mc = curl_multi_wait( multi, nullptr, 0, 10000, nullptr );
        if( CURLM_OK != mc ) {
            LOGGER_ERROR( cc::error( "Error: curl_multi_perform failed: " ) + cc::note( curl_multi_strerror( mc ) ) );
            continue;
        }

        timeval timeout = { 0, 0 };
        mc = getCurlMultiTimeout( timeout );
        if( mc != CURLM_OK )
            LOGGER_ERROR( cc::error( "curl_multi_timeout() failed: " ) + cc::note( curl_multi_strerror( mc ) ) );

        waitForDataReady( timeout, stillAlive );

        handleMessages();

        // add-remove easy-handles to/from multi-handle
        if( int recordsAdded = clearQueue() ) {
            mc = curl_multi_perform( m_multi.get(), &stillAlive );
            repeats = 0;
        }

        if( stillAlive ) {
            mc = curl_multi_wait( multi, nullptr, 0, 1000, nullptr );
            if( mc != CURLM_OK ) {
                LOGGER_ERROR( cc::error( "curl_multi_wait() failed: " ) + cc::note( curl_multi_strerror( mc ) ) );
            }

            repeats = 0;

            time( &wasStillAlive );
        }
        else {
            repeats++;

            const int repeatAtLeast = 10;
            shouldContinue = repeats < repeatAtLeast || ( time( nullptr ) - wasStillAlive <= exitDelayInterval() );
        }
    } while( ( stillAlive || shouldContinue ) && !m_connections.isStopped() );

    LOGGER_TRACE( cc::trace( "Exit from HttpMultiClient thread" ) );
}

CURLMcode HttpMultiClient::getCurlMultiTimeout( timeval& timeout ) {
    /* set a suitable timeout to play around with */
    timeout.tv_sec = 30000;
    timeout.tv_usec = 0;
    long curlTimeout = -1;
    auto mc = curl_multi_timeout( m_multi.get(), &curlTimeout );
    if( mc == CURLM_OK ) {
        if( curlTimeout >= 0 ) {
            timeout.tv_sec = curlTimeout / 1000;
            if( timeout.tv_sec > 1 )
                timeout.tv_sec = 1;
            else
                timeout.tv_usec = ( curlTimeout % 1000 ) * 1000;
        }
    }
    return mc;
}

int HttpMultiClient::waitForDataReady( timeval& timeout, int& stillAlive ) {
    int maxfd = -1;
    fd_set fdread;
    fd_set fdwrite;
    fd_set fdexcep;
    FD_ZERO( &fdread );
    FD_ZERO( &fdwrite );
    FD_ZERO( &fdexcep );
    CURLMcode mc = curl_multi_fdset( m_multi.get(), &fdread, &fdwrite, &fdexcep, &maxfd );
    if( mc != CURLM_OK ) {
        LOGGER_ERROR( cc::error( "curl_multi_fdset() failed: " ) + cc::note( curl_multi_strerror( mc ) ) );
    }

    int rc;
    if( maxfd == -1 ) {
        struct timeval wait = { 0, 100 * 1000 }; /* 100ms */
        rc = select( 0, nullptr, nullptr, nullptr, &wait );
    }
    else {
        rc = select( maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout );
    }

    switch( rc ) {
    case -1:
        LOGGER_DEBUG_RAW( "select() failed (-1)" );
    case 0:  /* timeout */
    default: /* action */
        rc = curl_multi_perform( m_multi.get(), &stillAlive );
        if( stillAlive && rc != 0 ) {
            LOGGER_TRACE_RAW( "curl_multi_perform() stillAlive: " + cc::num10( stillAlive ) +
                              ", resultCode: " + cc::num( rc ) );
        }
    }

    return rc;
}

void HttpMultiClient::handleMessages() {
    CURLMsg* msg = nullptr;
    int messagesLeft = 0;
    while( ( msg = curl_multi_info_read( m_multi.get(), &messagesLeft ) ) != nullptr ) {
        char* connectionSign = nullptr;  // pluginId:connectionId
        curl_easy_getinfo( msg->easy_handle, CURLINFO_PRIVATE, &connectionSign );
        long httpCode = 0;
        curl_easy_getinfo( msg->easy_handle, CURLINFO_RESPONSE_CODE, &httpCode );

        std::string pluginId, connId;
        ConnectionPtr connectionPtr;
        if( connectionSign && Connection::fromSign( connectionSign, pluginId, connId ) ) {
            connectionPtr = m_connections.findConnection( pluginId, connId );
            if( connectionPtr ) {
                if( httpCode != 100 ) {
                    connectionPtr->response().setHttpStatus( static_cast< int >( httpCode ) );
                }
            }
            else {
                LOGGER_ERROR( cc::error( "Connection not found by sign: " ) + connectionSign );
            }
        }
        else {
            connectionPtr = m_connections.findByHandle( msg->easy_handle );
            if( !connectionPtr ) {
                LOGGER_ERROR( cc::error( "Connection without a sign not found by handle: " ) + connectionPtr->getId() );
            }
        }

        if( msg->msg == CURLMSG_DONE ) {
            if( msg->data.result != CURLE_OK ) {
                Log::print( Log::ERROR,
                            "R: %d - %s <%s>, http code: %u\n",
                            msg->data.result,
                            curl_easy_strerror( msg->data.result ),
                            connectionSign ? connectionSign : "",
                            httpCode );
                if( connectionPtr ) {
                    Log::print(
                        Log::ERROR, "  Connection error: <%s>: '%s'\n", connectionSign, connectionPtr->errorBuffer() );
                    Reason reason = getReason( msg, connectionPtr.get() );
                    connectionPtr->emitConnectionClosed( static_cast< int >( reason ), connectionPtr->errorBuffer() );
                }
            }

            if( connectionPtr ) {
                Connection& conn = *connectionPtr;
                const HttpOptions& options = conn.getOptions();
                if( options.repeatAllowed )
                    m_connectionQueue.askRemoveFromMultiHandle( conn.getPluginId(), conn.getId() );

                if( msg->data.result != CURLE_OK ) {
                    m_connections.closeConnection( pluginId, connId, false );  // already removed
                }
                else if( msg->data.result == CURLE_OK ) {
                    if( !options.last ) {
                        conn.emitDataSend();
                    }
                }
            }
        }
        else {
            Log::print( Log::ERROR, "E: CURLMsg (%d)\n", msg->msg );
        }
    }
}

/** Adds and removes all queued connections to/from multi handle in curl's own thread.
 * Call it only from run() method in curl_multi handling thread
 * @returns number of connections added to multi-handle
 */
unsigned HttpMultiClient::clearQueue() {
    PluginConnectionQueue::ConnectionIdPairList copyToAdd;
    PluginConnectionQueue::ConnectionIdPairList copyToRemove;
    PluginConnectionQueue::ConnectionMap copyMarkedForRemoval;

    m_connectionQueue.extractCollections( copyToAdd, copyToRemove, copyMarkedForRemoval );
    unsigned recordsAdded = 0;

    for( const auto& idPair : copyToAdd ) {
        const auto& pluginId = idPair.first;
        const auto& connId = idPair.second;
        if( auto ptr = m_connections.findConnection( pluginId, connId ) ) {
            try {
                addMultiHandle( *ptr );
                recordsAdded++;
            }
            catch( const HttpError& ex ) {
                LOGGER_ERROR( cc::error( "Error adding connection with id " + ptr->getId() + " to multi-handle: " ) +
                              cc::note( ex.what() ) );
                // Retry by adding the pair to the end of a queue to add
                m_connectionQueue.pushBackToAdd( idPair );
            }
        }
        else {
            if( copyMarkedForRemoval.find( connId ) == copyMarkedForRemoval.cend() ) {
                LOGGER_ERROR( cc::error( "No connection to add found in a plugin queue: " ) + "(" +
                              cc::note( pluginId ) + ", " + cc::note( connId ) + ")" );
            }
        }
    }
    copyToAdd.clear();

    std::list< ConnectionPtr > toFlush;
    for( const auto& idPair : copyToRemove ) {
        const auto& pluginId = idPair.first;
        const auto& connId = idPair.second;
        if( auto ptr = m_connections.findConnection( pluginId, connId ) ) {
            try {
                removeMultiHandle( *ptr );
                /** Flush buffer when easy-handle outside of multi-handle to enable repeat_request()
                 *     call from http_data_received event handler */
                toFlush.emplace_front( ptr );
            }
            catch( const HttpError& ex ) {
                LOGGER_ERROR( cc::error( "Error removing connection " + connId + " from multi-handle: " ) +
                              cc::note( ex.what() ) );
                m_connectionQueue.pushBackToRemove( idPair );
            }
        }
        else {
            auto foundIt = copyMarkedForRemoval.find( connId );
            if( foundIt == copyMarkedForRemoval.cend() ) {
                LOGGER_ERROR( cc::error( "No connection to remove found in a plugin queue: " ) + "(" +
                              cc::note( pluginId ) + ", " + cc::note( connId ) + ")" );
            }
        }
    }

    if( !copyMarkedForRemoval.empty() )
        LOGGER_DEBUG_RAW( "Explicitly remove from multi-handle all marked for removal connections: " +
                          cc::num10( copyMarkedForRemoval.size() ) );
    for( auto& it : copyMarkedForRemoval ) {
        const auto& connId = it.first;
        auto& connectionPtr = it.second;

        try {
            removeMultiHandle( *connectionPtr );
        }
        catch( const HttpError& ex ) {
            LOGGER_ERROR(
                cc::error( "Error removing marked for removal connection " + connId + " from multi-handle: " ) +
                cc::note( ex.what() ) );
        }
    }
    copyMarkedForRemoval.clear();

    // flush connections outside locked scope
    for( const auto& ptr : toFlush ) {
        ptr->flushBuffer( true );

        if( !ptr->getOptions().keepAlive ) {
            try {
                m_connections.closeConnection( ptr->getPluginId(), ptr->getId(), false );  // already removed
            }
            catch( const HttpError& ex ) {
                LOGGER_ERROR( cc::error( "Connection already closed: " ) +
                              cc::note( ptr->getPluginId() + ":" + ptr->getId() ) );
            }
        }
        else {
            LOGGER_WARN_RAW( cc::warn( "Keep-alive connection is not closed: connId: " ) + ptr->getId() );
        }
    }
    toFlush.clear();
    return recordsAdded;
}

int HttpMultiClient::exitDelayInterval() const {
    return m_exitDelayInterval;
}

void HttpMultiClient::exitDelayInterval( int value ) {
    m_exitDelayInterval = value;
}

}  // namespace CommonHttp
}  // namespace HubPlatform