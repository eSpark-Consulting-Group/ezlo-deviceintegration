#include "hub_protect_security_config_handler.h"
#include "cored/remote_interface/cored_interface_factory.h"

namespace HubPlatform {
namespace Uid {

HubProtectSecurityConfigHandler::HubProtectSecurityConfigHandler( Service& service )
: m_service( service ) {}

void HubProtectSecurityConfigHandler::handleSecurityConfigList( rpc::Message& rpcMessage ) {
    auto& answer = rpcMessage.getAnswer();
    const auto& params = rpcMessage.getParams();
    auto core = HubPlatform::CoreDRemoteInterface::makeInstance( m_service );
    auto result = core->securityConfigOperations()->list( params );
    answer[ "result" ] = result;
}

bool HubProtectSecurityConfigHandler::isBypassSet( const std::string& securityConfigId ) {
    auto core = HubPlatform::CoreDRemoteInterface::makeInstance( m_service );
    return core->securityConfigOperations()->isBypassSet( securityConfigId );
}

}  // namespace Uid
}  // namespace HubPlatform