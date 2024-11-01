
#include "uid/nma_proto.h"
#include "uid/nma_client.h"
#include "uid/firmware.h"
#include "uid/ezlo.h"
#include "uid/broadcast.h"
#include "uid/ui_service.h"
#include "uid/handlers/hub_offline_commands_handler.h"

#include "common_utils/firmware_info_helper.h"
#include "base_utils/ezlodb.h"
#include "base_utils/utils.h"
#include "database_client/db_tables_names.h"

namespace HubPlatform {
namespace Uid {

using namespace HubPlatform::HardwareManager;

void NmaProto::registered( rpc::Message& rpcMessage ) {
    JsonObject& joAnswer = rpcMessage.getAnswer();
    std::string serverUrl = cc::debug( "NMA server" );

    if( auto nmaClient = getService().getNmaClient() ) {
        nmaClient->setHubRegistered( true );
        serverUrl = cc::u( nmaClient->uri() );
    }

    //
    // l_sergiy: update "cache-owner-info" state
    std::string strOwnerEmail, strHash;
    if( joAnswer.count( "result" ) > 0 && joAnswer[ "result" ].type() == Json::value_t::object ) {
        const JsonObject& joAnswerResult = joAnswer[ "result" ];
        strOwnerEmail = getFieldSafe< std::string >( joAnswerResult, "cache-owner" );
        strHash = getFieldSafe< std::string >( joAnswerResult, "cache-hash" );
        if( ( !strOwnerEmail.empty() ) && ( !strHash.empty() ) ) {
            auto joCacheOwner = Db::findOne( HaCacheDBName, JSON( "cache-owner-info" << 1 ) );
            if( joCacheOwner.empty() ) {
                Log::info( cc::info( "cache-owner-info" ) + cc::debug( " was " ) + cc::error( "not found" ) +
                           cc::debug( " in DB, will insert" ) );
                Db::insert(
                    HaCacheDBName,
                    JSON( "cache-owner-info" << 1 << "cache-owner" << strOwnerEmail << "cache-hash" << strHash ) );
            }
            else if( joCacheOwner.count( "cache-owner" ) == 0 || joCacheOwner.count( "cache-hash" ) == 0 ||
                     joCacheOwner[ "cache-owner" ].type() != Json::value_t::string ||
                     joCacheOwner[ "cache-hash" ].type() != Json::value_t::string ||
                     getFieldSafe< std::string >( joCacheOwner, "cache-owner" ) != strOwnerEmail ||
                     getFieldSafe< std::string >( joCacheOwner, "cache-hash" ) != strHash ) {
                Log::info( cc::info( "cache-owner-info" ) + cc::debug( " was " ) + cc::success( "found" ) +
                           cc::debug( " in DB, will update" ) );
                Db::update( HaCacheDBName,
                            JSON( "cache-owner-info" << 1 ),
                            JSON( "$set" << JSON( "cache-owner" << strOwnerEmail << "cache-hash" << strHash ) ) );
            }
            else
                Log::info( cc::info( "cache-owner-info" ) + cc::debug( " was " ) + cc::success( "found" ) +
                           cc::debug( " in DB, do not need to update it" ) );
        }  // if( (!strOwnerEmail.empty()) && (!strHash.empty()) )
    }
    //

    std::string strInfoMsg = cc::success( "Registered" ) + cc::info( " on " ) + serverUrl;
    if( !strOwnerEmail.empty() ) {
        strInfoMsg += cc::info( ", cached " ) + cc::note( "owner" ) + cc::info( " is " ) + cc::eml( strOwnerEmail );
        if( !strHash.empty() )
            strInfoMsg += cc::info( ", cached " ) + cc::note( "hash" ) + cc::info( " is " ) + cc::warn( strHash );
    }
    Log::info( strInfoMsg );
    joAnswer = nullptr;
}

void NmaProto::sendLogToServer( rpc::Message& rpcMessage ) {
    auto response = getService().sendRequest( "backupd", rpcMessage.getParams(), SEND_LOG_TO_SERVER );
    if( hasError( response ) ) {
        auto& answer = rpcMessage.getAnswer();
        answer[ "error" ] = std::move( response[ "error" ] );
    }
    else {
        Broadcast::sendToUis( LOG_FILE_UPLOADED, response );
    }
}

void NmaProto::getLogTail( rpc::Message& rpcMessage ) {
    auto response = getService().sendRequest( "backupd", rpcMessage.getParams(), SEND_LOG_TAIL );
    if( hasError( response ) ) {
        auto& answer = rpcMessage.getAnswer();
        answer[ "error" ] = std::move( response[ "error" ] );
    }
    else {
        Broadcast::sendToUis( LOG_FILE_UPLOADED, response );
    }
}

void NmaProto::updateUsers( rpc::Message& rpcMessage ) {
    Log::warn( cc::warn( "Deprecated method was executed: " ) + cc::note( "updateUsers" ) );
    JsonObject& joAnswer = rpcMessage.getAnswer() = nullptr;
}

void NmaProto::handleCloudAccessKeysControllerSync( rpc::Message& rpcMessage ) {
    HubOfflineCommandsHandler( getService() ).handleCloudAccessKeysControllerSyncRequest( rpcMessage );
}

bool NmaProto::invokeHandler( const std::string& method, rpc::Message& message ) {
    using RequestHandler = void ( NmaProto::* )( rpc::Message& );
    using RequestHandlersMap = std::unordered_map< std::string, RequestHandler >;
    static const RequestHandlersMap handlers = { { "registered", &NmaProto::registered },
                                                 { "register", &NmaProto::registered },
                                                 { "sendLogToServer", &NmaProto::sendLogToServer },
                                                 { "getLogTail", &NmaProto::getLogTail },
                                                 { "updateUsers", &NmaProto::updateUsers },
                                                 { "cloud.access_keys_controller_sync",
                                                   &NmaProto::handleCloudAccessKeysControllerSync },
                                                 { "hub.device.firmware.update", &NmaProto::updateDeviceFirmware } };

    const auto it = handlers.find( method );
    if( it != handlers.end() ) {
        const auto handler = it->second;
        ( this->*handler )( message );
        return true;
    }
    else {
        return Proto::invokeHandler( method, message );
    }
}

}  // namespace Uid
}  // namespace HubPlatform
