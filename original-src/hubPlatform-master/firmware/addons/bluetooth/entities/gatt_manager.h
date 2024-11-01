#pragma once

#include "bluetooth/proxies/gatt_manager_proxy.h"

#include <string>
#include <memory>

namespace HubPlatform {

class GattManager final {
public:
    class Service final {
    public:
        Service( const std::string& serviceUUID );

        gboolean getPrimaryProperty( const GDBusPropertyTable* property, DBusMessageIter* iter );
        gboolean getUUIDProperty( const GDBusPropertyTable* property, DBusMessageIter* iter );

    private:
        std::string m_serviceUUID;
    };

    class Characteristic final {
    public:
        Characteristic( const std::string& servicePath, const std::string& charPath, const std::string& serviceUUID );

        DBusMessage* readValue( DBusConnection* connection, DBusMessage* message );
        DBusMessage* writeValue( DBusConnection* connection, DBusMessage* message );
        DBusMessage* startNotify( DBusConnection* connection, DBusMessage* message );
        DBusMessage* stopNotify( DBusConnection* connection, DBusMessage* message );

        gboolean getUUIDProperty( const GDBusPropertyTable* property, DBusMessageIter* iter );
        gboolean getServiceProperty( const GDBusPropertyTable* property, DBusMessageIter* iter );
        gboolean getFlagsProperty( const GDBusPropertyTable* property, DBusMessageIter* iter );

    private:
        std::string m_servicePath;
        std::string m_charPath;
        std::string m_serviceUUID;
    };

    GattManager( DBusConnection* connection, const GattManagerProxy& proxy );

    void registerApplication( const std::string& serviceUUID, const std::string& characterUUID );

private:
    DBusConnection* m_connection;
    GattManagerProxy m_gattManagerProxy;
    std::unique_ptr< Service > m_service;
    std::unique_ptr< Characteristic > m_characteristic;
    const std::string m_gattAppObjetPath = "/";
    bool m_registered = false;
};

}  // namespace HubPlatform
