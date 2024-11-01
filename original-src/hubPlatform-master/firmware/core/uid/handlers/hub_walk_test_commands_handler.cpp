#include "uid/handlers/hub_walk_test_commands_handler.h"
#include "uid/nma_client.h"
#include "service_interfaces/service.h"
#include "uid/ui_service.h"
#include "cored/remote_interface/cored_interface_factory.h"
#include "cored/remote_interface/cored_remote_interface.h"
#include "cored/model/resources/cored_resources.h"

namespace HubPlatform {
namespace Uid {

Service& HubWalkTestCommandsHandler::getService() {
    return m_service;
}

HubWalkTestCommandsHandler::HubWalkTestCommandsHandler( Uid::Service& service )
: m_service( service ) {}

void HubWalkTestCommandsHandler::handleProtectSystemTest( rpc::Message& rpcMessage ) {
    using namespace HubPlatform::CoreD::Model::Resources;
    auto& answer = rpcMessage.getAnswer();
    const auto& params = rpcMessage.getParams();
    auto core = HubPlatform::CoreDRemoteInterface::makeInstance( getService() );
    auto result = core->walkTest()->protectSystemTestStart( params );
    answer[ "result" ] = result;
}

void HubWalkTestCommandsHandler::handleProtectSystemTestStop( rpc::Message& rpcMessage ) {
    using namespace HubPlatform::CoreD::Model::Resources;
    auto& answer = rpcMessage.getAnswer();
    const auto& params = rpcMessage.getParams();
    auto core = HubPlatform::CoreDRemoteInterface::makeInstance( getService() );
    auto result = core->walkTest()->protectSystemTestStop( params );
    answer[ "result" ] = result;
}

void HubWalkTestCommandsHandler::handleProtectSystemTestExtend( rpc::Message& rpcMessage ) {
    using namespace HubPlatform::CoreD::Model::Resources;
    auto& answer = rpcMessage.getAnswer();
    const auto& params = rpcMessage.getParams();
    auto core = HubPlatform::CoreDRemoteInterface::makeInstance( getService() );
    auto result = core->walkTest()->protectSystemTestExtend( params );
    answer[ "result" ] = result;
}

void HubWalkTestCommandsHandler::invoke( const std::string& method, rpc::Message& rpcMessage ) {
    using SimpleHandler = void ( HubWalkTestCommandsHandler::* )( rpc::Message& );
    using SimpleHandlersMap = std::unordered_map< std::string, SimpleHandler >;

    static const SimpleHandlersMap handlers = {
        { "protect.system.test.start", &HubWalkTestCommandsHandler::handleProtectSystemTest },
        { "protect.system.test.stop", &HubWalkTestCommandsHandler::handleProtectSystemTestStop },
        { "protect.system.test.extend", &HubWalkTestCommandsHandler::handleProtectSystemTestExtend }
    };

    const auto it = handlers.find( method );
    if( it != handlers.end() ) {
        const auto handler = it->second;
        ( this->*handler )( rpcMessage );
        return;
    }

    JsonObject& joAnswer = rpcMessage.getAnswer();
    setError( joAnswer, Errors::UNKNOWN_METHOD );
}

}  // namespace Uid
}  // namespace HubPlatform
