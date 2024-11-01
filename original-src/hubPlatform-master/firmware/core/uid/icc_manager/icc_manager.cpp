#include "uid/icc_manager/icc_manager.h"
#include "uid/icc_manager/controller_raw_command_builder.h"
#include "uid/icc_interface/icc_common.h"
#include "uid/ui_service.h"
#include "uid/nma_client.h"

namespace HubPlatform {
namespace InterControllerCommunication {

const auto MaxMessageIdLifetime = std::chrono::seconds( 30 );  // Same as DefaultICCRequestTimeout

IccManager::IccManager( Uid::Service& uiService )
: m_uiService( uiService ) {}

bool IccManager::processDirectCmd( rpc::Message& rpcMessage, const std::string& controllerUuid ) {
    JsonObject& joMessage = rpcMessage.getMessage();
    if( rpcMessage.getId() == "ui_broadcast" ) {
        joMessage[ "sender" ] = JSON( "type"
                                      << "controller"
                                      << "uuid" << controllerUuid );

        rpcMessage.getAnswer() = nullptr;
        m_uiService.sendBroadcast( EXTERNAL_CONTROLLERS_BROADCAST, joMessage, GroupsNames::EXTERNAL_CONTROLLERS );
        return true;
    }

    if( joMessage.count( "result" ) != 0 ) {
        rpcMessage.getAnswer() = nullptr;
        if( hasError( joMessage ) ) {
            joMessage[ "result" ][ "error" ] = std::move( joMessage[ "error" ] );
        }

        return sendAsyncResponse( rpcMessage.getId(), joMessage, rpcMessage.getId() );
    }

    return false;
}

bool IccManager::processCloudCmd( rpc::Message& rpcMessage ) {
    const JsonObject& joMessage = rpcMessage.getMessage();
    if( rpcMessage.getId() == "ui_broadcast" ) {
        rpcMessage.getAnswer() = nullptr;

        auto uuid = json_get_str( joMessage[ "sender" ], "uuid" );
        auto controllerId = Common::getControllerIdByUUID( m_uiService.getDBClient(), uuid );
        if( !controllerId.empty() ) {
            if( !m_uiService.getClusterManager().hasControllerConnection( controllerId ) ) {
                m_uiService.sendBroadcast(
                    EXTERNAL_CONTROLLERS_BROADCAST, joMessage, GroupsNames::EXTERNAL_CONTROLLERS );
            }
        }
        return true;
    }

    using RequestHandler = void ( IccManager::* )( rpc::Message& );
    using RequestHandlersMap = std::unordered_map< std::string, RequestHandler >;
    static const RequestHandlersMap handlers = {
        { "cloud.controller_raw_command", &IccManager::handleCloudControllerResponse },
    };

    std::string method = rpcMessage.getMethod();
    const auto it = handlers.find( method );
    if( it != handlers.end() ) {
        const auto handler = it->second;
        ( this->*handler )( rpcMessage );
        return true;
    }
    else {
        return false;
    }
}

void IccManager::handleCloudControllerResponse( rpc::Message& rpcMessage ) {
    rpcMessage.getAnswer() = nullptr;
    const JsonObject& joMessage = rpcMessage.getMessage();

    if( hasError( joMessage ) ) {
        sendAsyncResponse( rpcMessage.getId(), JSON( "error" << std::move( joMessage[ "error" ] ) ), "" );
        return;
    }

    static const std::string errorNameBase = "controller_raw_command.";
    auto makeErrorName = []( const char* error ) -> std::string { return errorNameBase + error; };

    if( joMessage.count( "result" ) == 0 ) {
        sendAsyncResponse( rpcMessage.getId(),
                           JSON( "error" << JSON( "code"
                                                  << "-35461"
                                                  << "data" << makeErrorName( "malformed_response" ) << "description"
                                                  << "" ) ),
                           "" );
        return;
    }

    auto& result = joMessage[ "result" ];

    auto actionUsedIt = result.find( "action_used" );
    if( actionUsedIt == result.end() || !actionUsedIt->is_string() ) {
        sendAsyncResponse( rpcMessage.getId(),
                           JSON( "error" << JSON( "code"
                                                  << "-35462"
                                                  << "data" << makeErrorName( "malformed_response" ) << "description"
                                                  << "" ) ),
                           "" );
        return;
    }
    if( actionUsedIt.value() != "instant" ) {
        sendAsyncResponse( rpcMessage.getId(), JSON_EMPTY_OBJ, "" );
        return;
    }

    auto responseIt = result.find( "controller_response" );
    if( responseIt == result.end() ) {
        sendAsyncResponse( rpcMessage.getId(),
                           JSON( "error" << JSON( "code"
                                                  << "-35460"
                                                  << "data" << makeErrorName( "malformed_result" ) << "description"
                                                  << "" ) ),
                           "" );
        return;
    }

    auto command = responseIt.value();
    if( command.count( "id" ) == 0 ) {
        sendAsyncResponse( rpcMessage.getId(),
                           JSON( "error" << JSON( "code"
                                                  << "-35459"
                                                  << "data" << makeErrorName( "malformed_payload" ) << "description"
                                                  << "" ) ),
                           "" );
        return;
    }

    sendAsyncResponse( rpcMessage.getId(), command, command[ "id" ] );
}

bool IccManager::sendAsyncResponse( const std::string& messageId, const Json& message, const std::string& uuid ) {
    auto it = m_senderByMessageIds.find( messageId );
    if( it == m_senderByMessageIds.end() ) {
        Log::error( cc::error( "Unexpected response id: " ) + cc::note( messageId ) +
                    cc::error( ", probably request was already timed out" ) );
        return false;
    }

    auto& sender = it->second.first;
    auto& cachedUuid = it->second.second;

    if( !uuid.empty() && uuid != cachedUuid ) {
        Log::error( cc::error( "Internal and external uuid consistency check failed" ) );
    }

    m_uiService.sendAsyncResponse( sender, message, cachedUuid );
    m_senderByMessageIds.erase( messageId );
    cleanOldMessageIds();
    return true;
}

// request = {
//    "controllerId": 70010999,
//    "payload": { Actual request object }
// }
void IccManager::handleRequest( const JsonObject& request, const std::string& uuid, const std::string& sender ) {
    Json payload = request[ "payload" ];
    if( payload.count( "id" ) == 0 ) {
        payload[ "id" ] = uuid;
    }

    std::string messageId;
    auto controllerId = request[ "controllerId" ];

    if( m_uiService.getClusterManager().postMessage( controllerId, payload ) ) {
        messageId = json_get_str( payload, "id" );
    }
    else {
        messageId = Services::Service::generateMessageUUID();

        const std::string controllerUuid = Common::getControllerUUIDById( m_uiService.getDBClient(), controllerId );
        ControllerRawCommandBuilder commandBuilder( std::move( payload ), controllerUuid );
        commandBuilder.setInstant( true );
        commandBuilder.setQueued( false );

        const Json message = commandBuilder.build( messageId );
        m_uiService.getNmaClient()->postMessage( message );
    }

    cleanOldMessageIds();
    m_senderByMessageIds.emplace( messageId, std::make_pair( sender, uuid ) );
    m_messageIdsWithTimestamps.emplace_back( messageId, std::chrono::steady_clock::now() );
}

void IccManager::cleanOldMessageIds() {
    if( m_messageIdsWithTimestamps.empty() ) {
        return;
    }

    auto now = std::chrono::steady_clock::now();

    while( !m_messageIdsWithTimestamps.empty() ) {
        auto& messageId = m_messageIdsWithTimestamps.front().first;
        auto& messageTimestamp = m_messageIdsWithTimestamps.front().second;
        if( now - messageTimestamp < MaxMessageIdLifetime ) {
            break;
        }
        m_senderByMessageIds.erase( messageId );
        m_messageIdsWithTimestamps.pop_front();
    }
}

}  // namespace InterControllerCommunication
}  // namespace HubPlatform
