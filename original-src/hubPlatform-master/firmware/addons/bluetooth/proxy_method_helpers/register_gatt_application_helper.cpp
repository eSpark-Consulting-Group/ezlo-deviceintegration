#include "bluetooth/proxy_method_helpers/register_gatt_application_helper.h"

namespace HubPlatform {

RegisterGattApplicationHelper::RegisterGattApplicationHelper( Proxy& proxy, const std::string& path )
: ProxyAsyncMethodHelper( proxy, "RegisterApplication", true )
, m_path( path ) {}

void RegisterGattApplicationHelper::setup( DBusMessageIter* iter ) {
    const char* path = m_path.c_str();
    DBusMessageIter dict;

    dbus_message_iter_append_basic( iter, DBUS_TYPE_OBJECT_PATH, &path );
    dbus_message_iter_open_container( iter, DBUS_TYPE_ARRAY, "{sv}", &dict );
    dbus_message_iter_close_container( iter, &dict );
}

}  // namespace HubPlatform
