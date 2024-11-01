#include "uid/server/server_proto.h"
#include "uid/handlers/hub_offline_commands_handler.h"
#include "uid/cluster_manager/cluster_manager.h"
#include "uid/ezlo.h"
#include "version/ha_version.h"
#include "database_client/dbqueryparser.h"
#include "database_client/db_flags_names.h"
#include "base_utils/ezlodb.h"

server_proto::server_proto()
: Parser(
      {
          { "hub.offline.login.ui", loginUser },
          { "hub.offline.login.hub", loginController },
      },
      unknown ) {}

bool server_proto::allowEveryone( const std::string& strMethodName ) {
    static const std::set< std::string > methods = { "hub.info.get" };
    return methods.find( strMethodName ) != methods.end();
}

bool server_proto::processCmd( server_peer_message& rpcMessage ) {
    const JsonObject& joMessage = rpcMessage.getMessage();
    JsonObject& joAnswer = rpcMessage.getAnswer();
    std::string method = rpcMessage.getMethod();
    if( method.empty() ) {
        setError( joAnswer, Errors::MESSAGE_HAS_EMPTY_METHOD );
        Log::fatal( cc::fatal( "\"method\" is empty in JSON (LocalNMA proto)" ) );
        return false;
    }
    std::string id = rpcMessage.getId();
    if( id.empty() )
        Log::warn( cc::warn( "\"id\" is empty in JSON for method \"" + method + "\" (LocalNMA proto)" ) );
    JsonObject joSender = JSON_EMPTY_OBJ;
    json_add_null( joAnswer, Errors::ERROR_FIELD );
    if( json_get_obj( joMessage, "sender", joSender ) )
        joAnswer[ "sender" ] = joSender;
    if( !id.empty() )
        json_add_str( joAnswer, "id", id );
    Log::info( cc::info( "UI daemon will process command " ) + cc::warn( method ) );
    if( joMessage.count( "result" ) > 0 && joMessage[ "result" ].type() == Json::value_t::object )
        joAnswer[ "result" ] = joMessage[ "result" ];
    else
        joAnswer[ "result" ] = JSON_EMPTY_OBJ;
    getHandler( method )( rpcMessage );
    if( joAnswer.is_object() && ( !joAnswer[ std::string( Errors::ERROR_FIELD ) ].is_null() ) ) {
        if( !rpcMessage.getPeerObject()->isLoggedIn() && !Db::getFlag( OfflineAnonymousAccessFlagName ) ) {
            // NOTE: spam protection from non-logged peers
            std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
        }
        joAnswer.erase( "result" );
    }
    return true;
}

void server_proto::unknown( server_peer_message& rpcMessage ) {
    std::cout.flush();
    std::cerr.flush();
    std::cerr << "\nVERY BAD, UNKNOWN_METHOD, LocalNMA proto\n\n";
    std::cout.flush();
    std::cerr.flush();
    JsonObject& joAnswer = rpcMessage.getAnswer();
    setError( joAnswer, Errors::UNKNOWN_METHOD );
}

void server_proto::loginUser( server_peer_message& rpcMessage ) {
    const JsonObject& joParams = rpcMessage.getParams();
    if( rpcMessage.isLoggedIn() ) {
        rpcMessage.setError( Errors::USER_HAS_LOGGED );
        return;
    }
    if( !checkField( joParams, "user", rpcMessage.getAnswer() ) ||
        !checkField( joParams, "token", rpcMessage.getAnswer() ) ) {
        return;
    }
    std::string user = getFieldSafe< std::string >( joParams, "user" );
    std::string token = getFieldSafe< std::string >( joParams, "token" );
    if( user.empty() ) {
        rpcMessage.setError( Errors::WRONG_PARAMS );
        return;
    }

    if( !HubPlatform::Uid::HubOfflineCommandsHandler::authenticateUser( user, token ) ) {
        rpcMessage.setError( Errors::BAD_PASSWORD );
        return;
    }

    // success, remember userId
    rpcMessage.getPeerObject()->onLoginConfirmed( user, true );
}

void server_proto::loginController( server_peer_message& rpcMessage ) {
    const JsonObject& joParams = rpcMessage.getParams();
    if( rpcMessage.isLoggedIn() ) {
        rpcMessage.setError( Errors::USER_HAS_LOGGED );
        return;
    }
    if( !checkField( joParams, "hub", rpcMessage.getAnswer() ) ||
        !checkField( joParams, "token", rpcMessage.getAnswer() ) ) {
        return;
    }
    std::string hub = getFieldSafe< std::string >( joParams, "hub" );
    std::string token = getFieldSafe< std::string >( joParams, "token" );
    if( hub.empty() ) {
        rpcMessage.setError( Errors::WRONG_PARAMS );
        return;
    }

    if( !HubPlatform::Uid::HubOfflineCommandsHandler::authenticateController( hub, token ) ) {
        rpcMessage.setError( Errors::BAD_PASSWORD );
        return;
    }

    if( auto clusterManagerPtr = HubPlatform::Uid::ClusterManager::getInstance() ) {
        if( !clusterManagerPtr->onExternalControllerLoggingIn( hub, rpcMessage.getPeerObject() ) ) {
            rpcMessage.setError( Errors::USER_HAS_LOGGED );
            return;
        }
    }
    else {
        Log::error( cc::error( "Cluster manager is not initialized" ) );
    }

    // success, remember controller uuid
    rpcMessage.getPeerObject()->onLoginConfirmed( hub, false );
}
