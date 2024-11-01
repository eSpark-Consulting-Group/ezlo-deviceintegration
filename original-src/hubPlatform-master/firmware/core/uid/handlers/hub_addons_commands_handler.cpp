#include "uid/handlers/hub_addons_commands_handler.h"
#include "uid/nma_client.h"

namespace HubPlatform {
namespace Uid {

HubAddonsCommandsHandler::HubAddonsCommandsHandler( Uid::Service& service )
: m_service( service ) {}

void HubAddonsCommandsHandler::invoke( const std::string& method, rpc::Message& rpcMessage ) {
    LOGGER_TRACE( cc::trace( "invoke method: " ) + cc::note( method ) );
    JsonObject& joMessage = rpcMessage.getMessage();
    rpcMessage.answerPrepare();
    rpcMessage.copySender();
    auto& joAnswer = rpcMessage.getAnswer();
    auto peer = rpcMessage.getNetworkObject();
    if( joMessage.count( "api" ) != 0 ) {
        joAnswer[ "api" ] = std::move( joMessage[ "api" ] );
    }

    auto addonCommandPos = method.find( '.' );
    if( addonCommandPos != std::string::npos ) {
        const auto addonId = method.substr( 0, addonCommandPos );
        const auto commandId = method.substr( addonCommandPos + 1 );
        m_service.sendAsyncRequest(
            "addon." + addonId, joMessage, ADDON_MESSAGE, [joAnswer, peer]( const JsonObject& response ) mutable {
                setErrorIfHas( joAnswer, response );
                joAnswer[ "result" ] = std::move( response );
                json_remove( joAnswer[ "result" ], Errors::ERROR_FIELD );
                if( peer ) {
                    peer->sendMessage( joAnswer );
                }
            } );
    }
    joAnswer = nullptr;
}

}  // namespace Uid
}  // namespace HubPlatform
