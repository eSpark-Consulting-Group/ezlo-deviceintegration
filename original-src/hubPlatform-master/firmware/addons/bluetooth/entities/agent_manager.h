#pragma once

#include "bluetooth/proxies/agent_manager_proxy.h"

#include <memory>

namespace HubPlatform {

class AgentManager final {
public:
    class Agent final {
    public:
        explicit Agent( const std::string& path );

        DBusMessage* release( DBusConnection* connection, DBusMessage* message );
        DBusMessage* requestPinCode( DBusConnection* connection, DBusMessage* message );
        DBusMessage* displayPinCode( DBusConnection* connection, DBusMessage* message );
        DBusMessage* requestPasskey( DBusConnection* connection, DBusMessage* message );
        DBusMessage* displayPasskey( DBusConnection* connection, DBusMessage* message );
        DBusMessage* requestConfirmation( DBusConnection* connection, DBusMessage* message );
        DBusMessage* requestAuthorization( DBusConnection* connection, DBusMessage* message );
        DBusMessage* authorizeService( DBusConnection* connection, DBusMessage* message );
        DBusMessage* cancel( DBusConnection* connection, DBusMessage* message );

    private:
        std::string m_path;
    };

    AgentManager( DBusConnection* connection, const AgentManagerProxy& proxy );

    enum class AgentCapability { DisplayOnly, DisplayYesNo, KeyboardOnly, NoInputNoOutput, KeyboardDisplay };

    void registerAgent( AgentCapability capability );
    void unregisterAgent();

private:
    DBusConnection* m_connection;
    AgentManagerProxy m_agentManagerProxy;
    std::unique_ptr< Agent > m_agent;
    const std::string m_agentManagerObjetPath = "/org/bluez/agent";

    bool m_registered = false;
};

}  // namespace HubPlatform
