#include "hub_protect_command_id_handler.h"
#include "cored/remote_interface/cored_interface_factory.h"

namespace HubPlatform {
namespace Uid {

HubProtectCommandIdHandler::HubProtectCommandIdHandler( Service& service )
: m_service( service ) {}

void HubProtectCommandIdHandler::handleCommandIdList( rpc::Message& rpcMessage ) {
    auto& answer = rpcMessage.getAnswer();
    const auto& params = rpcMessage.getParams();
    auto core = HubPlatform::CoreDRemoteInterface::makeInstance( m_service );
    auto result = core->contactIdOperations()->list( params );
    answer[ "result" ] = result;
}
}  // namespace Uid
}  // namespace HubPlatform