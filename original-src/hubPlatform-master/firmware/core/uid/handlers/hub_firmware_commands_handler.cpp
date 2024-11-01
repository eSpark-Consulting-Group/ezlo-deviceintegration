#include "uid/handlers/hub_firmware_commands_handler.h"
#include "uid/ui_service.h"

#include "update_manager/remote_interface/um_remote_interface.h"

#include "base_utils/acl.h"

namespace HubPlatform {
namespace Uid {

HubFirmwareCommandsHandler::HubFirmwareCommandsHandler( Services::Service& hostService )
: m_updateManager( hostService ) {}

void HubFirmwareCommandsHandler::invoke( const std::string& method, rpc::Message& rpcMessage ) {
    using RequestHandler = void ( HubFirmwareCommandsHandler::* )( rpc::Message & rpcMessage );
    using RequestHandlersMap = std::unordered_map< std::string, RequestHandler >;
    static const RequestHandlersMap requestHandlers = {
        { "luup.switch", &HubFirmwareCommandsHandler::handleLuupSwitchRequest },
    };

    const auto it = requestHandlers.find( method );
    if( it != requestHandlers.end() ) {
        const auto handler = it->second;
        ( this->*handler )( rpcMessage );
        return;
    }

    JsonObject& joAnswer = rpcMessage.getAnswer();
    setError( joAnswer, Errors::UNKNOWN_METHOD );
}

// params : {}
void HubFirmwareCommandsHandler::handleLuupSwitchRequest( rpc::Message& rpcMessage ) {
    JsonObject& joAnswer = rpcMessage.getAnswer();
    if( !Acl::check( rpcMessage.getParams(), Resource::EZLO, Permission::EXECUTE, joAnswer ) )
        return;
    /* TODO: this operation makes sense only on g* hubs, perform such a check */

    m_updateManager.executeLuupSwitch();
}

}  // namespace Uid
}  // namespace HubPlatform
