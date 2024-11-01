#include "bluetooth/entities/advertising_manager.h"

#include "bluetooth/bluez_interfaces.h"
#include "bluetooth/utils/macros.h"
#include "bluetooth/log.h"

#include "common_utils/cpp14.h"

namespace HubPlatform {

namespace {
const GDBusMethodTable AdvertisingMethods[] = {
    { METHOD_MACRO( "Release", nullptr, nullptr, AdvertisingManager::Advertisement, release ) },
    {}
};

const GDBusPropertyTable AdvertisingProperties[] = {
    { PROPERTY_MACRO( "Type", "s", AdvertisingManager::Advertisement, getTypeProperty ) },
    { PROPERTY_MACRO( "ServiceUUIDs", "as", AdvertisingManager::Advertisement, getServiceUUIDsProperty ) },
    { PROPERTY_MACRO( "Discoverable", "b", AdvertisingManager::Advertisement, getDiscoverableProperty ) },
    { PROPERTY_MACRO( "Includes", "as", AdvertisingManager::Advertisement, getIncludesProperty ) },
    { PROPERTY_MACRO( "LocalName", "s", AdvertisingManager::Advertisement, getLocalNameProperty ) },
    {}
};
}  // namespace

AdvertisingManager::Advertisement::Advertisement( const std::string& path,
                                                  const std::string& localName,
                                                  const std::vector< std::string >& advertiseServiceUUIDs )
: m_path( path )
, m_localName( localName )
, m_advertiseServiceUUIDs( advertiseServiceUUIDs ) {}

DBusMessage* AdvertisingManager::Advertisement::release( DBusConnection* connection, DBusMessage* message ) {
    g_dbus_unregister_interface( connection, m_path.c_str(), BluezInterfaces::LEAdvertisingManager );

    return dbus_message_new_method_return( message );
}

gboolean AdvertisingManager::Advertisement::getTypeProperty( const GDBusPropertyTable* property,
                                                             DBusMessageIter* iter ) {
    const char* type = "peripheral";

    dbus_message_iter_append_basic( iter, DBUS_TYPE_STRING, &type );

    return true;
}

gboolean AdvertisingManager::Advertisement::getServiceUUIDsProperty( const GDBusPropertyTable* property,
                                                                     DBusMessageIter* iter ) {
    DBusMessageIter array;

    dbus_message_iter_open_container( iter, DBUS_TYPE_ARRAY, "as", &array );

    for( const auto& serviceUuid : m_advertiseServiceUUIDs ) {
        auto uuid = serviceUuid.c_str();
        dbus_message_iter_append_basic( &array, DBUS_TYPE_STRING, &uuid );
    }

    dbus_message_iter_close_container( iter, &array );

    return true;
}
gboolean AdvertisingManager::Advertisement::getDiscoverableProperty( const GDBusPropertyTable* property,
                                                                     DBusMessageIter* iter ) {
    const dbus_bool_t discoverable = true;

    dbus_message_iter_append_basic( iter, DBUS_TYPE_BOOLEAN, &discoverable );

    return true;
}

gboolean AdvertisingManager::Advertisement::getIncludesProperty( const GDBusPropertyTable* property,
                                                                 DBusMessageIter* iter ) {
    DBusMessageIter array;
    const char* str = "tx-power";

    dbus_message_iter_open_container( iter, DBUS_TYPE_ARRAY, "as", &array );
    dbus_message_iter_append_basic( &array, DBUS_TYPE_STRING, &str );
    dbus_message_iter_close_container( iter, &array );

    return true;
}

gboolean AdvertisingManager::Advertisement::getLocalNameProperty( const GDBusPropertyTable* property,
                                                                  DBusMessageIter* iter ) {
    dbus_message_iter_append_basic( iter, DBUS_TYPE_STRING, &m_localName );

    return true;
}

AdvertisingManager::AdvertisingManager( DBusConnection* connection, const AdvertisingManagerProxy& proxy )
: m_connection( connection )
, m_advertisingManagerProxy( proxy ) {}

void AdvertisingManager::startAdvertising( const std::string& localName,
                                           const std::vector< std::string >& advertiseServiceUUIDs ) {
    if( m_advertising ) {
        return;
    }

    auto advertisement =
        std::make_unique< Advertisement >( m_advertisingManagerObjetPath, localName, advertiseServiceUUIDs );

    if( g_dbus_register_interface( m_connection,
                                   m_advertisingManagerObjetPath.c_str(),
                                   BluezInterfaces::LEAdvertisingManager,
                                   AdvertisingMethods,
                                   nullptr,
                                   AdvertisingProperties,
                                   advertisement.get(),
                                   nullptr ) == false ) {
        LOG_ERROR( "Failed to register an advertising" );

        return;
    }

    m_advertisingManagerProxy.startAdvertising( m_advertisingManagerObjetPath );

    m_advertisement = std::move( advertisement );
    m_advertising = true;

    LOG_INFO( "Advertising is registered" );
}

void AdvertisingManager::stopAdvertising() {
    if( !m_advertising ) {
        return;
    }

    m_advertisingManagerProxy.stopAdvertising( m_advertisingManagerObjetPath );

    g_dbus_unregister_interface(
        m_connection, m_advertisingManagerObjetPath.c_str(), BluezInterfaces::LEAdvertisingManager );

    m_advertisement.reset();
    m_advertising = false;

    LOG_INFO( "Advertising is unregistered" );
}

}  // namespace HubPlatform
