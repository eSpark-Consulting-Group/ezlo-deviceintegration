#include "uid/handlers/hub_swinnger_shutdown_commands_handler.h"
#include "uid/nma_client.h"
#include "service_interfaces/service.h"
#include "uid/ui_service.h"
#include "cored/remote_interface/cored_interface_factory.h"
#include "cored/remote_interface/cored_remote_interface.h"
#include "cored/model/resources/cored_resources.h"

namespace HubPlatform {
namespace Uid {

Service& HubSwingerShutdownCommandsHandler::getService() {
    return m_service;
}

HubSwingerShutdownCommandsHandler::HubSwingerShutdownCommandsHandler( Uid::Service& service )
: m_service( service ) {}

void HubSwingerShutdownCommandsHandler::handleSwingerShutdownList( rpc::Message& rpcMessage ) {
    using namespace HubPlatform::CoreD::Model::Resources;
    auto& answer = rpcMessage.getAnswer();
    const auto& params = rpcMessage.getParams();
    auto core = HubPlatform::CoreDRemoteInterface::makeInstance( getService() );
    auto result = core->swingerShutdown()->getSwingerShutdownList( params );
    answer[ "result" ] = result;
}

void HubSwingerShutdownCommandsHandler::handleSwingerShutdownReset( rpc::Message& rpcMessage ) {
    using namespace HubPlatform::CoreD::Model::Resources;
    auto& answer = rpcMessage.getAnswer();
    const auto& params = rpcMessage.getParams();
    auto core = HubPlatform::CoreDRemoteInterface::makeInstance( getService() );
    auto result = core->swingerShutdown()->resetDeviceStatistics( params );
    answer[ "result" ] = result;
}

void HubSwingerShutdownCommandsHandler::handleSwingerShutdownDisableAdd( rpc::Message& rpcMessage ) {
    using namespace HubPlatform::CoreD::Model::Resources;
    auto& answer = rpcMessage.getAnswer();
    const auto& params = rpcMessage.getParams();
    auto core = HubPlatform::CoreDRemoteInterface::makeInstance( getService() );
    auto result = core->swingerShutdown()->disableAdd( params );
    answer[ "result" ] = result;
}

void HubSwingerShutdownCommandsHandler::handleSwingerShutdownDisableRemove( rpc::Message& rpcMessage ) {
    using namespace HubPlatform::CoreD::Model::Resources;
    auto& answer = rpcMessage.getAnswer();
    const auto& params = rpcMessage.getParams();
    auto core = HubPlatform::CoreDRemoteInterface::makeInstance( getService() );
    auto result = core->swingerShutdown()->disableRemove( params );
    answer[ "result" ] = result;
}

void HubSwingerShutdownCommandsHandler::handleSwingerShutdownLimitsSet( rpc::Message& rpcMessage ) {
    using namespace HubPlatform::CoreD::Model::Resources;
    auto& answer = rpcMessage.getAnswer();
    const auto& params = rpcMessage.getParams();
    auto core = HubPlatform::CoreDRemoteInterface::makeInstance( getService() );
    auto result = core->swingerShutdown()->limitsSet( params );
    answer[ "result" ] = result;
}

void HubSwingerShutdownCommandsHandler::handleSwingerShutdownLimitsReset( rpc::Message& rpcMessage ) {
    using namespace HubPlatform::CoreD::Model::Resources;
    auto& answer = rpcMessage.getAnswer();
    const auto& params = rpcMessage.getParams();
    auto core = HubPlatform::CoreDRemoteInterface::makeInstance( getService() );
    auto result = core->swingerShutdown()->limitsReset( params );
    answer[ "result" ] = result;
}

void HubSwingerShutdownCommandsHandler::invoke( const std::string& method, rpc::Message& rpcMessage ) {
    using SimpleHandler = void ( HubSwingerShutdownCommandsHandler::* )( rpc::Message& );
    using SimpleHandlersMap = std::unordered_map< std::string, SimpleHandler >;

    static const SimpleHandlersMap handlers = {
        { "swinger.shutdown.list", &HubSwingerShutdownCommandsHandler::handleSwingerShutdownList },
        { "swinger.shutdown.reset", &HubSwingerShutdownCommandsHandler::handleSwingerShutdownReset },
        { "swinger.shutdown.disable.add", &HubSwingerShutdownCommandsHandler::handleSwingerShutdownDisableAdd },
        { "swinger.shutdown.disable.remove", &HubSwingerShutdownCommandsHandler::handleSwingerShutdownDisableRemove },
        { "swinger.shutdown.limits.set", &HubSwingerShutdownCommandsHandler::handleSwingerShutdownLimitsSet },
        { "swinger.shutdown.limits.reset", &HubSwingerShutdownCommandsHandler::handleSwingerShutdownLimitsReset },
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
