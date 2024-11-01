#pragma once

#include "extensions_manager/remote_interface/em_remote_interface.h"

#include "jsonrpc/message.h"

#include <string>

namespace HubPlatform {
namespace Uid {

class HubExtensionsCommandsHandler final {
public:
    HubExtensionsCommandsHandler( Services::Service& hostService );

    void invoke( const std::string& method, rpc::Message& rpcMessage );

private:
    void handlePluginRun( rpc::Message& rpcMessage );
    void handleConfigList( rpc::Message& rpcMessage );
    void handleConfigPrioritySet( rpc::Message& rpcMessage );

private:
    ExtensionsManager::RemoteInterface m_extensionsManager;
};

}  // namespace Uid
}  // namespace HubPlatform
