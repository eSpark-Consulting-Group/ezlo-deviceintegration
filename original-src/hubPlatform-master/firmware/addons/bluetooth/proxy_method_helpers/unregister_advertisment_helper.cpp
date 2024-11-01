#include "bluetooth/proxy_method_helpers/unregister_advertisment_helper.h"

#include "bluetooth/log.h"

namespace HubPlatform {

UnregisterAdvertismentHelper::UnregisterAdvertismentHelper( Proxy& proxy, const std::string& path )
: ProxyAsyncMethodHelper( proxy, "UnregisterAdvertisement", true )
, m_path( path ) {}

void UnregisterAdvertismentHelper::setup( DBusMessageIter* iter ) {
    const char* path = m_path.c_str();
    dbus_message_iter_append_basic( iter, DBUS_TYPE_OBJECT_PATH, &path );
}

}  // namespace HubPlatform
