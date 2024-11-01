
#include "service_interfaces/script_client.h"

#include "addon_interfaces/addon.h"
#include "jsonrpc/jsonrpc.h"

namespace HubPlatform {

ScriptClient::ScriptClient( std::string remoteAddonQualifiedName, Services::Service& host )
: m_host( host )
, m_remoteAddonQualifiedName( std::move( remoteAddonQualifiedName ) ) {}

ScriptClient::~ScriptClient() = default;

void ScriptClient::sendMethodCall( Json data ) const {
    Services::Message message{ m_remoteAddonQualifiedName, JSON_RPC, CALL_METHOD, std::move( data ) };
    getHost().send( message );
}

Json ScriptClient::sendMethodCall( MessageSubClasses messageSubClasses, Json&& data ) const {
    return getHost().sendMethodCall( m_remoteAddonQualifiedName, messageSubClasses, std::move( data ) );
}

Json ScriptClient::sendRequest( MessageSubClasses messageSubClasses, Json&& data ) const {
    return getHost().sendRequest( m_remoteAddonQualifiedName, messageSubClasses, std::move( data ) );
}

Services::Service& ScriptClient::getHost() const {
    return m_host;
}

}  // namespace HubPlatform
