#include "bluetooth/proxy_method_helpers/request_default_agent_helper.h"

namespace HubPlatform {

RequestDefaultAgentHelper::RequestDefaultAgentHelper( Proxy& proxy, const std::string& path )
: ProxyAsyncMethodHelper( proxy, "RequestDefaultAgent", true )
, m_path( path ) {}

void RequestDefaultAgentHelper::setup( DBusMessageIter* iter ) {
    auto path = m_path.c_str();

    dbus_message_iter_append_basic( iter, DBUS_TYPE_OBJECT_PATH, &path );
}

}  // namespace HubPlatform
