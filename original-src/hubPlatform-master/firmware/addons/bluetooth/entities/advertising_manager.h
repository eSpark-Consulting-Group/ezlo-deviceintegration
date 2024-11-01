#pragma once

#include "bluetooth/proxies/advertising_manager_proxy.h"

#include <memory>
#include <string>

namespace HubPlatform {

class AdvertisingManager final {
public:
    class Advertisement final {
    public:
        Advertisement( const std::string& path,
                       const std::string& localName,
                       const std::vector< std::string >& advertiseServiceUUIDs );

        DBusMessage* release( DBusConnection* connection, DBusMessage* message );

        gboolean getTypeProperty( const GDBusPropertyTable* property, DBusMessageIter* iter );
        gboolean getServiceUUIDsProperty( const GDBusPropertyTable* property, DBusMessageIter* iter );
        gboolean getDiscoverableProperty( const GDBusPropertyTable* property, DBusMessageIter* iter );
        gboolean getIncludesProperty( const GDBusPropertyTable* property, DBusMessageIter* iter );
        gboolean getLocalNameProperty( const GDBusPropertyTable* property, DBusMessageIter* iter );

    private:
        std::string m_path;
        std::string m_localName;
        std::vector< std::string > m_advertiseServiceUUIDs;
    };

    AdvertisingManager( DBusConnection* connection, const AdvertisingManagerProxy& proxy );

    void startAdvertising( const std::string& localName, const std::vector< std::string >& advertiseServiceUUIDs );
    void stopAdvertising();

private:
    DBusConnection* m_connection;
    AdvertisingManagerProxy m_advertisingManagerProxy;
    std::unique_ptr< Advertisement > m_advertisement;
    const std::string m_advertisingManagerObjetPath = "/org/bluez/advertising";

    bool m_advertising = false;
};

}  // namespace HubPlatform
