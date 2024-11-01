#include "bluetooth/proxies/agent_manager_proxy.h"

#include "bluetooth/proxy_method_helpers/register_agent_helper.h"
#include "bluetooth/proxy_method_helpers/request_default_agent_helper.h"
#include "bluetooth/proxy_method_helpers/unregister_agent_helper.h"

namespace HubPlatform {

AgentManagerProxy::AgentManagerProxy( GDBusProxy* proxy )
: Proxy( proxy ) {}

void AgentManagerProxy::registerAgent( const std::string& path, const std::string& capability ) {
    RegisterAgentHelper( *this, path, capability ).callProxyMethod();
}

void AgentManagerProxy::requestDefaultAgent( const std::string& path ) {
    RequestDefaultAgentHelper( *this, path ).callProxyMethod();
}

void AgentManagerProxy::unregisterAgent( const std::string& path ) {
    UnregisterAgentHelper( *this, path ).callProxyMethod();
}

}  // namespace HubPlatform
