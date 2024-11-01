#include "bluetooth/proxy_method_helpers/unregister_agent_helper.h"

namespace HubPlatform {

UnregisterAgentHelper::UnregisterAgentHelper( Proxy& proxy, const std::string& path )
: ProxyAsyncMethodHelper( proxy, "UnregisterAgent", true )
, m_path( path ) {}

void UnregisterAgentHelper::setup( DBusMessageIter* iter ) {
    auto path = m_path.c_str();

    dbus_message_iter_append_basic( iter, DBUS_TYPE_OBJECT_PATH, &path );
}

}  // namespace HubPlatform
