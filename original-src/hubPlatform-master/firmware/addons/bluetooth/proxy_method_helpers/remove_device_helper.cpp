#include "bluetooth/proxy_method_helpers/remove_device_helper.h"

namespace HubPlatform {

RemoveDeviceHelper::RemoveDeviceHelper( Proxy& proxy, std::string deviceProxyPath )
: ProxyAsyncMethodHelper( proxy, "RemoveDevice", true )
, m_deviceProxyPath( std::move( deviceProxyPath ) ) {}

void RemoveDeviceHelper::setup( DBusMessageIter* iter ) {
    auto pathPtr = m_deviceProxyPath.c_str();
    dbus_message_iter_append_basic( iter, DBUS_TYPE_OBJECT_PATH, const_cast< char** >( &pathPtr ) );
}

}  // namespace HubPlatform
