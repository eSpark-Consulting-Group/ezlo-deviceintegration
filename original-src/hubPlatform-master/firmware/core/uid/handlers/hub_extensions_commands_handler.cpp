#include "uid/handlers/hub_extensions_commands_handler.h"
#include "uid/ui_service.h"

#include "update_manager/remote_interface/um_remote_interface.h"

#include "base_utils/acl.h"

namespace HubPlatform {
namespace Uid {

HubExtensionsCommandsHandler::HubExtensionsCommandsHandler( Services::Service& hostService )
: m_extensionsManager( hostService ) {}

void HubExtensionsCommandsHandler::invoke( const std::string& method, rpc::Message& rpcMessage ) {
    using RequestHandler = void ( HubExtensionsCommandsHandler::* )( rpc::Message & rpcMessage );
    using RequestHandlersMap = std::unordered_map< std::string, RequestHandler >;
    static const RequestHandlersMap requestHandlers = {
        { "plugin.run", &HubExtensionsCommandsHandler::handlePluginRun },
        { "config.list", &HubExtensionsCommandsHandler::handleConfigList },
        { "config.priority.set", &HubExtensionsCommandsHandler::handleConfigPrioritySet },
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

// "params": {
//        "script": "HUB:zwave/scripts/init",
//        "scriptParams": {},
//        ...
// TODO:  "code": "..."
//    }
void HubExtensionsCommandsHandler::handlePluginRun( rpc::Message& rpcMessage ) {
    const JsonObject& message = rpcMessage.getMessage();
    auto params = message[ "params" ];

    JsonObject& joAnswer = rpcMessage.getAnswer();
    if( !Acl::check( rpcMessage.getParams(), Resource::EZLO, Permission::EXECUTE, joAnswer ) )
        return;

    auto scriptName = json_get_str( params, "script" );
    Json scriptParams = JSON_EMPTY_OBJ;
    json_get_obj( params, "scriptParams", scriptParams );
    std::string operationId = rpcMessage.getId();

    if( scriptName.empty() ) {
        setError( joAnswer, Errors::BAD_PARAMS );
        return;
    }

    auto error = m_extensionsManager.requestPluginRun( scriptName, scriptParams, rpcMessage.getId() );

    if( !error.empty() ) {
        joAnswer[ "error" ] = std::move( error );
        return;
    }
}

void HubExtensionsCommandsHandler::handleConfigList( rpc::Message& rpcMessage ) {
    const JsonObject& message = rpcMessage.getMessage();

    JsonObject& joAnswer = rpcMessage.getAnswer();
    if( !Acl::check( rpcMessage.getParams(), Resource::EZLO, Permission::VIEW, joAnswer ) )
        return;

    const auto list = m_extensionsManager.requestConfigList();
    if( list.empty() ) {
        setError( joAnswer, Errors::BAD_PARAMS );
        return;
    }

    joAnswer[ "result" ] = list;
}

void HubExtensionsCommandsHandler::handleConfigPrioritySet( rpc::Message& rpcMessage ) {
    const JsonObject& message = rpcMessage.getMessage();
    auto params = message[ "params" ];

    JsonObject& joAnswer = rpcMessage.getAnswer();
    if( !Acl::check( rpcMessage.getParams(), Resource::EZLO, Permission::EDIT, joAnswer ) )
        return;

    auto pluginId = json_get_str( params, "plugin" );
    auto priorities = json_get_vec< std::string >( params, "priorities" );

    if( pluginId.empty() || priorities.empty() ) {
        setError( joAnswer, Errors::BAD_PARAMS );
        return;
    }

    const auto prioritiesList = m_extensionsManager.requestConfigPrioritySet( pluginId, priorities );
    if( prioritiesList.empty() ) {
        setError( joAnswer, Errors::BAD_PARAMS );
        return;
    }

    joAnswer[ "result" ] = prioritiesList;
}

}  // namespace Uid
}  // namespace HubPlatform
