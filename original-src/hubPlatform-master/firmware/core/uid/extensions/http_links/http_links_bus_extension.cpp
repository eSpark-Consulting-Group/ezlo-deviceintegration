#include "uid/extensions/http_links/http_links_bus_extension.h"
#include "uid/extensions/http_links/errors.h"
#include "uid/extensions/http_links/resources.h"
#include "jsonrpc/jsonrpc.h"

namespace HubPlatform {
namespace HttpLinks {

HttpLinksBusExtension::HttpLinksBusExtension( HttpLinksManager& manager )
: m_manager( manager ) {}

void HttpLinksBusExtension::handleMethodCall( const JsonObject& json,
                                              MessageSubClasses requestType,
                                              const std::string& sender ) {}

JsonObject HttpLinksBusExtension::handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) {
    using MethodHandler = JsonObject ( HttpLinksBusExtension::* )( const JsonObject& json );
    using MethodHandlersMap = std::unordered_map< MessageSubClasses, MethodHandler, MessageSubClassesHash >;
    static const MethodHandlersMap methodHandlers = {
        { HTTP_LINKS_CREATE_LINK_URL, &HttpLinksBusExtension::handleCreateLinkUrl },
        { HTTP_LINKS_DELETE_LINK_URL, &HttpLinksBusExtension::handleDeleteLinkUrl },
        { HTTP_LINKS_GET_LINK_INFO, &HttpLinksBusExtension::handleGetLinkInfo },
        { HTTP_LINKS_CLOSE_CONNECTION, &HttpLinksBusExtension::handleCloseConnection },
        { HTTP_LINKS_GET_CONNECTION_INFO, &HttpLinksBusExtension::handleGetConnectionInfo },
        { HTTP_LINKS_SEND_DATA, &HttpLinksBusExtension::handleSendData }
    };
    const auto it = methodHandlers.find( requestType );
    if( it != methodHandlers.end() ) {
        const auto handler = it->second;
        return ( this->*handler )( request );
    }

    return unhandledRequestAnswer();
}

/** @returns { result: { linkId: <linkIdValue>, url; string, secure_url: string }, error?: object  } */
JsonObject HttpLinksBusExtension::handleCreateLinkUrl( const JsonObject& json ) {
    Json answer = JSON_EMPTY_OBJ;
    try {
        HttpLink link;
        m_manager.fromJsonDb( link, json );

        auto linkId = m_manager.createHttpLink( link );
        answer[ "result" ][ "linkId" ] = linkId;
        if( !linkId.empty() ) {
            auto linkInfo = m_manager.getLinkInfo( link.pluginId(), linkId );
            jsonCopy( linkInfo, "url", answer[ "result" ] );
            jsonCopy( linkInfo, "secureUrl", answer[ "result" ], "secure_url" );
        }
    }
    catch( const HttpLinksError& ex ) {
        LOGGER_ERROR( cc::error( "Exception: " ) + cc::note( ex.what() ) );
        setError( answer, Errors::BAD_PARAMS, ex.what() );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Exception: " ) + cc::note( ex.what() ) );
        setError( answer, Errors::INTERNAL_OPERATION_ERROR, ex.what() );
    }

    return answer;
}

/** @param json { linkId: string, pluginId: string }
 *  @returns { result: { deleted: bool }, error?: object  }
 */
JsonObject HttpLinksBusExtension::handleDeleteLinkUrl( const JsonObject& json ) {
    Json answer = JSON_EMPTY_OBJ;
    auto pluginId = json_get_str( json, HttpLinks::Resources::HttpLinkFields::pluginId );
    auto linkId = json_get_str( json, "linkId" );

    try {
        bool result = m_manager.deleteHttpLink( pluginId, linkId );
        answer[ "result" ][ "deleted" ] = result;
    }
    catch( const HttpLinksError& ex ) {
        LOGGER_ERROR( cc::error( "Exception: " ) + cc::note( ex.what() ) );
        setError( answer, Errors::BAD_PARAMS, ex.what() );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Exception: " ) + cc::note( ex.what() ) );
        setError( answer, Errors::INTERNAL_OPERATION_ERROR, ex.what() );
    }

    return answer;
}

/** @param json { linkId: string, pluginId: string }
 *  @returns { result: { linkInfo: object }, error?: object  }
 */
JsonObject HttpLinksBusExtension::handleGetLinkInfo( const JsonObject& json ) {
    Json answer = JSON_EMPTY_OBJ;
    auto pluginId = json_get_str( json, HttpLinks::Resources::HttpLinkFields::pluginId );
    auto linkId = json_get_str( json, "linkId" );

    try {
        auto linkInfo = m_manager.getLinkInfo( pluginId, linkId );
        if( linkInfo.empty() )
            throw HttpLinksError( "Link does not exist" );
        answer[ "result" ][ "linkInfo" ] = linkInfo;
    }
    catch( const HttpLinksError& ex ) {
        LOGGER_ERROR( cc::error( "Exception: " ) + cc::note( ex.what() ) );
        setError( answer, Errors::BAD_PARAMS, ex.what() );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Exception: " ) + cc::note( ex.what() ) );
        setError( answer, Errors::INTERNAL_OPERATION_ERROR, ex.what() );
    }

    return answer;
}

JsonObject HttpLinksBusExtension::handleCloseConnection( const JsonObject& json ) {
    using namespace HttpLinks::Resources;

    Json answer = JSON_EMPTY_OBJ;
    auto pluginId = json_get_str( json, HttpLinkFields::pluginId );
    auto connId = json_get_str( json, HttpLinkFields::connectionId );

    try {
        ConnectionInfo connInfo;
        if( m_manager.getConnectionInfo( connId, connInfo ) ) {
            // make response, check pluginId of a link
            Json linkInfo = m_manager.getLinkInfoRaw( connInfo.linkId() );
            auto linkPluginId = json_get_str( linkInfo, HttpLinkFields::pluginId );
            if( linkPluginId != pluginId ) {
                LOGGER_ERROR( cc::error( "pluginId does not match: " ) + pluginId + " != " + linkPluginId );
                throw HttpLinksError( "Plugin can query only own its own connections" );
            }
        }
        else {
            throw HttpLinksError( "Connection does not exist" );
        }

        bool connectionClosed = m_manager.closeConnection( pluginId, connId );
        answer[ "result" ][ "connectionClosed" ] = connectionClosed;
    }
    catch( const HttpLinksError& ex ) {
        LOGGER_ERROR( cc::error( "Exception: " ) + cc::note( ex.what() ) );
        setError( answer, Errors::BAD_PARAMS, ex.what() );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Exception: " ) + cc::note( ex.what() ) );
        setError( answer, Errors::INTERNAL_OPERATION_ERROR, ex.what() );
    }

    return answer;
}

JsonObject HttpLinksBusExtension::handleSendData( const JsonObject& json ) {
    using namespace HttpLinks::Resources;

    Json answer = JSON_EMPTY_OBJ;
    auto pluginId = json_get_str( json, HttpLinkFields::pluginId );
    auto connId = json_get_str( json, "id" );

    try {
        ConnectionInfo connInfo;
        if( m_manager.getConnectionInfo( connId, connInfo ) ) {
            // make response, check pluginId of a link
            Json linkInfo = m_manager.getLinkInfoRaw( connInfo.linkId() );
            auto linkPluginId = json_get_str( linkInfo, HttpLinkFields::pluginId );
            if( linkPluginId != pluginId ) {
                LOGGER_ERROR( cc::error( "pluginId does not match: " ) + pluginId + " != " + linkPluginId );
                throw HttpLinksError( "Plugin can query only own its own connections" );
            }
        }
        else {
            throw HttpLinksError( "Connection does not exist" );
        }
        bool sendDataOk = m_manager.sendData( pluginId, connId, std::move( json ) );
        answer[ "result" ][ "sendData" ] = sendDataOk;
    }
    catch( const HttpLinksError& ex ) {
        LOGGER_ERROR( cc::error( "Exception: " ) + cc::note( ex.what() ) );
        setError( answer, Errors::BAD_PARAMS, ex.what() );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Exception: " ) + cc::note( ex.what() ) );
        setError( answer, Errors::INTERNAL_OPERATION_ERROR, ex.what() );
    }

    return answer;
}

JsonObject HttpLinksBusExtension::handleGetConnectionInfo( const JsonObject& json ) {
    using namespace HttpLinks::Resources;

    Json answer = JSON_EMPTY_OBJ;
    auto pluginId = json_get_str( json, HttpLinkFields::pluginId );
    auto connId = json_get_str( json, HttpLinkFields::connectionId );
    LOGGER_DEBUG( cc::debug( "Start executing getConnectionInfo method for connection ID: " ) + cc::note( connId ) );
    try {
        ConnectionInfo connInfo;
        if( m_manager.getConnectionInfo( connId, connInfo ) ) {
            // make response, check pluginId of a link
            Json linkInfo = m_manager.getLinkInfoRaw( connInfo.linkId() );
            auto linkPluginId = json_get_str( linkInfo, HttpLinkFields::pluginId );
            if( linkPluginId != pluginId ) {
                LOGGER_ERROR( "pluginId does not match: " + pluginId + " != " + linkPluginId );
                throw HttpLinksError( "Plugin can query only own its own connections" );
            }
        }
        else {
            throw HttpLinksError( "Connection does not exist" );
        }
        answer[ "result" ][ "connectionInfo" ] = connInfo.toJsonDb();
    }
    catch( const HttpLinksError& ex ) {
        LOGGER_ERROR( cc::error( "Exception raised for connection id( " + connId + " ): " ) + cc::note( ex.what() ) );
        setError( answer, Errors::BAD_PARAMS, ex.what() );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Exception raised for connection id( " + connId + " ): " ) + cc::note( ex.what() ) );
        setError( answer, Errors::INTERNAL_OPERATION_ERROR, ex.what() );
    }
    auto timestamp = std::chrono::system_clock::now().time_since_epoch();
    auto duration = std::chrono::duration_cast< std::chrono::seconds >( timestamp ).count();
    LOGGER_DEBUG( cc::debug( "Exiting the getConnectionInfo method for connection ID: " ) + cc::note( connId ) +
                  cc::debug( " at timestamp: " ) + cc::num( duration ) );
    return answer;
}

void HttpLinksBusExtension::handleEventFromBus( const JsonObject& event, MessageSubClasses requestType ) {
    switch( requestType ) {
    case NETWORK_BROADCAST_IP_ADDRESS_CHANGED:
        m_manager.updateCurrentIp( event );
        break;
    case LOCAL_SERVER_STARTED:
        m_manager.setHostPorts( json_get_int( event, "insecurePort", 0 ), json_get_int( event, "port", 0 ) );
    default:
        break;
    }
}

}  // namespace HttpLinks
}  // namespace HubPlatform