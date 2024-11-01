#include "bluetooth/proxy_method_helpers/write_characteristic_helper.h"

#include <dbus/dbus.h>

namespace HubPlatform {

WriteCharacteristicHelper::WriteCharacteristicHelper( Proxy& proxy, const std::vector< std::uint8_t >& data )
: ProxySyncMethodHelper( proxy, "WriteValue", true )
, m_data( data ) {}

void WriteCharacteristicHelper::setup( DBusMessageIter* iter ) {
    DBusMessageIter array, dict;

    dbus_message_iter_open_container( iter, DBUS_TYPE_ARRAY, "y", &array );
    auto dataPtr = m_data.data();
    dbus_message_iter_append_fixed_array( &array, DBUS_TYPE_BYTE, &dataPtr, m_data.size() );
    dbus_message_iter_close_container( iter, &array );

    dbus_message_iter_open_container( iter,
                                      DBUS_TYPE_ARRAY,
                                      DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING DBUS_TYPE_STRING_AS_STRING
                                          DBUS_TYPE_VARIANT_AS_STRING DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
                                      &dict );

    uint16_t offset = 0;
    g_dbus_dict_append_entry( &dict, "offset", DBUS_TYPE_UINT16, &offset );

    dbus_message_iter_close_container( iter, &dict );
}

}  // namespace HubPlatform
