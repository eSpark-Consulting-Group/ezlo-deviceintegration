#include "bluetooth/proxy_method_helpers/register_advertisment_helper.h"

namespace HubPlatform {

RegisterAdvertismentHelper::RegisterAdvertismentHelper( Proxy& proxy, const std::string& path )
: ProxyAsyncMethodHelper( proxy, "RegisterAdvertisement", true )
, m_path( path ) {}

void RegisterAdvertismentHelper::setup( DBusMessageIter* iter ) {
    DBusMessageIter dict;
    const char* path = m_path.c_str();

    dbus_message_iter_append_basic( iter, DBUS_TYPE_OBJECT_PATH, &path );
    dbus_message_iter_open_container( iter,
                                      DBUS_TYPE_ARRAY,
                                      DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING DBUS_TYPE_STRING_AS_STRING
                                          DBUS_TYPE_VARIANT_AS_STRING DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
                                      &dict );
    dbus_message_iter_close_container( iter, &dict );
}

}  // namespace HubPlatform
