#include "bluetooth/proxy_method_helpers/read_characteristic_helper.h"

#include "bluetooth/log.h"

#include <dbus/dbus.h>

namespace HubPlatform {

ReadCharacteristicHelper::ReadCharacteristicHelper( Proxy& proxy )
: ProxySyncMethodHelper( proxy, "ReadValue", true ) {}

void ReadCharacteristicHelper::setup( DBusMessageIter* iter ) {
    DBusMessageIter dict;

    dbus_message_iter_open_container( iter,
                                      DBUS_TYPE_ARRAY,
                                      DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING DBUS_TYPE_STRING_AS_STRING
                                          DBUS_TYPE_VARIANT_AS_STRING DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
                                      &dict );

    uint16_t offset = 0;
    g_dbus_dict_append_entry( &dict, "offset", DBUS_TYPE_UINT16, &offset );

    dbus_message_iter_close_container( iter, &dict );
}

void ReadCharacteristicHelper::parseResult( DBusMessage* message ) {
    DBusMessageIter iter;
    dbus_message_iter_init( message, &iter );
    if( dbus_message_iter_get_arg_type( &iter ) != DBUS_TYPE_ARRAY ) {
        LOG_ERROR( "Invalid data type to read" );
        return;
    }

    DBusMessageIter array;
    dbus_message_iter_recurse( &iter, &array );

    int len = -1;
    uint8_t* value = nullptr;
    dbus_message_iter_get_fixed_array( &array, &value, &len );

    if( len < 0 ) {
        LOG_ERROR( "Invalid data length" );
        return;
    }

    m_value.assign( value, value + len );
}

const std::vector< std::uint8_t >& ReadCharacteristicHelper::getResult() const {
    return m_value;
}

}  // namespace HubPlatform
