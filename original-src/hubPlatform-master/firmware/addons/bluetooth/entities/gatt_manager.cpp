#include "bluetooth/entities/gatt_manager.h"

#include "bluetooth/bluez_interfaces.h"
#include "bluetooth/utils/macros.h"
#include "bluetooth/log.h"

#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace {

int parse_value( DBusMessageIter* iter, const uint8_t** value, int* len ) {
    DBusMessageIter array;

    if( dbus_message_iter_get_arg_type( iter ) != DBUS_TYPE_ARRAY )
        return -EINVAL;

    dbus_message_iter_recurse( iter, &array );
    dbus_message_iter_get_fixed_array( &array, value, len );

    return 0;
}

const GDBusPropertyTable ServiceProperties[] = {
    { PROPERTY_MACRO( "Primary", "b", GattManager::Service, getPrimaryProperty ) },
    { PROPERTY_MACRO( "UUID", "s", GattManager::Service, getUUIDProperty ) },
    {}
};

const GDBusMethodTable PipeCharacteristicMethods[] = {
    { ASYNC_METHOD_MACRO( "ReadValue",
                          GDBUS_ARGS( { "options", "a{sv}" } ),
                          GDBUS_ARGS( { "value", "ay" } ),
                          GattManager::Characteristic,
                          readValue ) },
    { ASYNC_METHOD_MACRO( "WriteValue",
                          GDBUS_ARGS( { "value", "ay" }, { "options", "a{sv}" } ),
                          nullptr,
                          GattManager::Characteristic,
                          writeValue ) },
    { ASYNC_METHOD_MACRO( "StartNotify", nullptr, nullptr, GattManager::Characteristic, startNotify ) },
    { METHOD_MACRO( "StopNotify", nullptr, nullptr, GattManager::Characteristic, stopNotify ) },
    {}
};

const GDBusPropertyTable PipeCharacteristicProperties[] = {
    { PROPERTY_MACRO( "UUID", "s", GattManager::Characteristic, getUUIDProperty ) },
    { PROPERTY_MACRO( "Service", "o", GattManager::Characteristic, getServiceProperty ) },
    { PROPERTY_MACRO( "Flags", "as", GattManager::Characteristic, getFlagsProperty ) },
    {}
};
}  // namespace

GattManager::Service::Service( const std::string& serviceUUID )
: m_serviceUUID( serviceUUID ) {}

gboolean GattManager::Service::getPrimaryProperty( const GDBusPropertyTable* property, DBusMessageIter* iter ) {
    dbus_bool_t primary = true;

    dbus_message_iter_append_basic( iter, DBUS_TYPE_BOOLEAN, &primary );

    return true;
}

gboolean GattManager::Service::getUUIDProperty( const GDBusPropertyTable* property, DBusMessageIter* iter ) {
    auto uuid = m_serviceUUID.c_str();

    dbus_message_iter_append_basic( iter, DBUS_TYPE_STRING, &uuid );

    return true;
}

GattManager::Characteristic::Characteristic( const std::string& servicePath,
                                             const std::string& charPath,
                                             const std::string& serviceUUID )
: m_servicePath( servicePath )
, m_charPath( charPath )
, m_serviceUUID( serviceUUID ) {}

DBusMessage* GattManager::Characteristic::readValue( DBusConnection* connection, DBusMessage* message ) {
    DBusMessage* reply;
    DBusMessageIter iter;
    DBusMessageIter array;

    const char* value = "42";

    if( !dbus_message_iter_init( message, &iter ) ) {
        return g_dbus_create_error( message, DBUS_ERROR_INVALID_ARGS, "Invalid arguments" );
    }

    /* TODO: parse options */

    reply = dbus_message_new_method_return( message );
    if( !reply ) {
        return g_dbus_create_error( message, DBUS_ERROR_NO_MEMORY, "No Memory" );
    }

    dbus_message_iter_init_append( reply, &iter );

    dbus_message_iter_open_container( &iter, DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE_AS_STRING, &array );
    dbus_message_iter_append_fixed_array( &array, DBUS_TYPE_BYTE, &value, sizeof( value ) );
    dbus_message_iter_close_container( &iter, &array );

    return reply;
}

DBusMessage* GattManager::Characteristic::writeValue( DBusConnection* connection, DBusMessage* message ) {
    DBusMessageIter iter;
    const uint8_t* value;
    int len;

    dbus_message_iter_init( message, &iter );

    if( parse_value( &iter, &value, &len ) )
        return g_dbus_create_error( message, DBUS_ERROR_INVALID_ARGS, "Invalid arguments" );

    /* TODO: parse options */

    g_dbus_emit_property_changed( connection, m_charPath.c_str(), BluezInterfaces::GattCharacteristic, "Value" );

    return dbus_message_new_method_return( message );
}

DBusMessage* GattManager::Characteristic::startNotify( DBusConnection* connection, DBusMessage* message ) {
    return g_dbus_create_error( message, DBUS_ERROR_NOT_SUPPORTED, "Not Supported" );
}

DBusMessage* GattManager::Characteristic::stopNotify( DBusConnection* connection, DBusMessage* message ) {
    return g_dbus_create_error( message, DBUS_ERROR_NOT_SUPPORTED, "Not Supported" );
}

gboolean GattManager::Characteristic::getUUIDProperty( const GDBusPropertyTable* property, DBusMessageIter* iter ) {
    auto uuid = m_serviceUUID.c_str();

    dbus_message_iter_append_basic( iter, DBUS_TYPE_STRING, &uuid );

    return true;
}

gboolean GattManager::Characteristic::getServiceProperty( const GDBusPropertyTable* property, DBusMessageIter* iter ) {
    auto servicePath = m_servicePath.c_str();

    dbus_message_iter_append_basic( iter, DBUS_TYPE_OBJECT_PATH, &servicePath );

    return true;
}

gboolean GattManager::Characteristic::getFlagsProperty( const GDBusPropertyTable* property, DBusMessageIter* iter ) {
    const char* flags[] = { "read", "write", "notify" };
    DBusMessageIter array;

    dbus_message_iter_open_container( iter, DBUS_TYPE_ARRAY, DBUS_TYPE_STRING_AS_STRING, &array );

    for( auto flag : flags ) {
        dbus_message_iter_append_basic( &array, DBUS_TYPE_STRING, &flag );
    }

    dbus_message_iter_close_container( iter, &array );

    return true;
}

GattManager::GattManager( DBusConnection* connection, const GattManagerProxy& proxy )
: m_connection( connection )
, m_gattManagerProxy( proxy ) {}

void GattManager::registerApplication( const std::string& serviceUUID, const std::string& characterUUID ) {
    if( m_registered ) {
        return;
    }

    auto servicePath = m_gattAppObjetPath + "service001";
    auto service = std::make_unique< Service >( serviceUUID );

    if( !g_dbus_register_interface( m_connection,
                                    servicePath.c_str(),
                                    BluezInterfaces::GattService,
                                    nullptr,
                                    nullptr,
                                    ServiceProperties,
                                    service.get(),
                                    nullptr ) ) {
        LOG_ERROR( "Couldn't register a local GATT service" );
        return;
    }

    auto charPath = servicePath + "/characteristic001";
    auto characteristic = std::make_unique< Characteristic >( servicePath, charPath, characterUUID );

    if( !g_dbus_register_interface( m_connection,
                                    charPath.c_str(),
                                    BluezInterfaces::GattCharacteristic,
                                    PipeCharacteristicMethods,
                                    nullptr,
                                    PipeCharacteristicProperties,
                                    characteristic.get(),
                                    nullptr ) ) {
        g_dbus_unregister_interface( m_connection, servicePath.c_str(), BluezInterfaces::GattService );
        LOG_ERROR( "Couldn't register a characteristic interface for a local GATT service" );

        return;
    }

    m_gattManagerProxy.registerApplication( m_gattAppObjetPath );

    m_service = std::move( service );
    m_characteristic = std::move( characteristic );
    m_registered = true;

    LOG_INFO( "Local GATT app is registered" );
}

}  // namespace HubPlatform
