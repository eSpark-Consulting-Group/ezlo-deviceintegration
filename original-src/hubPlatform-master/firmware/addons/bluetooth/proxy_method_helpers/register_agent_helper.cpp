#include "bluetooth/proxy_method_helpers/register_agent_helper.h"

namespace HubPlatform {

RegisterAgentHelper::RegisterAgentHelper( Proxy& proxy, const std::string& path, const std::string& capability )
: ProxyAsyncMethodHelper( proxy, "RegisterAgent", true )
, m_path( path )
, m_capability( capability ) {}

void RegisterAgentHelper::setup( DBusMessageIter* iter ) {
    auto path = m_path.c_str();
    auto capability = m_capability.c_str();

    dbus_message_iter_append_basic( iter, DBUS_TYPE_OBJECT_PATH, &path );
    dbus_message_iter_append_basic( iter, DBUS_TYPE_STRING, &capability );
}

}  // namespace HubPlatform
