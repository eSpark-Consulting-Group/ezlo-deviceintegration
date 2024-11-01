#include "uid/handlers/hub_provision_info_commands_handler.h"
#include "uid/ui_service.h"

#include "base_utils/acl.h"
#include "provision_data_utils/provision_data_manager.h"
#include "system_utils/environment.h"

#include <unordered_map>

namespace HubPlatform {
namespace Uid {

HubProvisionInfoCommandsHandler::HubProvisionInfoCommandsHandler( Service& hostService )
: m_hostService( hostService )
, m_updateManager( hostService ) {}

void HubProvisionInfoCommandsHandler::invoke( const std::string& method, rpc::Message& rpcMessage ) {
    using RequestHandler = void ( HubProvisionInfoCommandsHandler::* )( rpc::Message & rpcMessage );
    using RequestHandlersMap = std::unordered_map< std::string, RequestHandler >;
    static const RequestHandlersMap requestHandlers = {
        { "set", &HubProvisionInfoCommandsHandler::handleSetRequest },
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

void HubProvisionInfoCommandsHandler::handleSetRequest( rpc::Message& rpcMessage ) {
    const JsonObject& params = rpcMessage.getParams();
    JsonObject& joAnswer = rpcMessage.getAnswer();
    if( !Acl::check( rpcMessage.getParams(), Resource::EZLO, Permission::SUPER, joAnswer ) )
        return;

    auto data = ProvisionDataUtils::getManager().getProvisionData();

    auto newConfigRev = json_get_int( params, "configRev", 0 );
    if( newConfigRev <= data.configVersion ) {
        JsonObject response = JSON( "isUpdated" << false << "configRev" << data.configVersion );
        joAnswer[ "result" ] = std::move( response );
        return;
    }

    using namespace ProvisionDataUtils;
    setNonEmpty( data.configVersion, newConfigRev );
    setNonEmpty( data.provisionServer, json_get_str( params, "provisionServer" ) );
    setNonEmpty( data.cloudServer, json_get_str( params, "cloudServer" ) );
    setNonEmpty( data.provisionToken, json_get_str( params, "provisionToken" ) );
    setNonEmpty( data.sslPrivateKey, json_get_str( params, "sslPrivateKey" ) );
    setNonEmpty( data.sslSharedKey, json_get_str( params, "sslSharedKey" ) );
    setNonEmpty( data.signingCaCertificate, json_get_str( params, "signingCaCertificate" ) );
    setNonEmpty( data.timeServers, json_get_vec< std::string >( params, "timeServer" ) );

    if( data.configVersion <= 0 || data.provisionServer.empty() || data.cloudServer.empty() ) {
        setError( joAnswer, Errors::BAD_PARAMS );
        return;
    }

    const auto updateStatus = m_hostService.updateAndApplyProvisioningWithDelay( data, std::chrono::seconds( 5 ) );
    if( updateStatus == ProvisionDataUtils::UpdateStatus::Failed ) {
        setError( joAnswer, Errors::INTERNAL_OPERATION_ERROR );
        return;
    }

    JsonObject response = JSON( "isUpdated" << true << "configRev" << data.configVersion );
    joAnswer[ "result" ] = std::move( response );
}

}  // namespace Uid
}  // namespace HubPlatform
