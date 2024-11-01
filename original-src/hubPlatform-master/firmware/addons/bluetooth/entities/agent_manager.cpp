#include "bluetooth/entities/agent_manager.h"

#include "bluetooth/bluez_interfaces.h"
#include "bluetooth/utils/macros.h"
#include "bluetooth/log.h"

#include "common_utils/cpp14.h"

#include <unordered_map>
#include <string>

namespace HubPlatform {

namespace {
std::string getString( AgentManager::AgentCapability capability ) {
    static const std::unordered_map< AgentManager::AgentCapability, std::string > map = {
        { AgentManager::AgentCapability::DisplayOnly, "DisplayOnly" },
        { AgentManager::AgentCapability::DisplayYesNo, "DisplayYesNo" },
        { AgentManager::AgentCapability::KeyboardOnly, "KeyboardOnly" },
        { AgentManager::AgentCapability::NoInputNoOutput, "NoInputNoOutput" },
        { AgentManager::AgentCapability::KeyboardDisplay, "KeyboardDisplay" },
    };

    auto it = map.find( capability );
    if( it != map.end() ) {
        return it->second;
    }

    return "KeyboardDisplay";
}

const GDBusMethodTable AgentMethods[] = { { METHOD_MACRO( "Release", nullptr, nullptr, AgentManager::Agent, release ) },
                                          { ASYNC_METHOD_MACRO( "RequestPinCode",
                                                                GDBUS_ARGS( { "device", "o" } ),
                                                                GDBUS_ARGS( { "pincode", "s" } ),
                                                                AgentManager::Agent,
                                                                requestPinCode ) },
                                          { METHOD_MACRO( "DisplayPinCode",
                                                          GDBUS_ARGS( { "device", "o" }, { "pincode", "s" } ),
                                                          nullptr,
                                                          AgentManager::Agent,
                                                          displayPinCode ) },
                                          { ASYNC_METHOD_MACRO( "RequestPasskey",
                                                                GDBUS_ARGS( { "device", "o" } ),
                                                                GDBUS_ARGS( { "passkey", "u" } ),
                                                                AgentManager::Agent,
                                                                requestPasskey ) },
                                          { METHOD_MACRO(
                                              "DisplayPasskey",
                                              GDBUS_ARGS( { "device", "o" }, { "passkey", "u" }, { "entered", "q" } ),
                                              nullptr,
                                              AgentManager::Agent,
                                              displayPasskey ) },
                                          { ASYNC_METHOD_MACRO( "RequestConfirmation",
                                                                GDBUS_ARGS( { "device", "o" }, { "passkey", "u" } ),
                                                                nullptr,
                                                                AgentManager::Agent,
                                                                requestConfirmation ) },
                                          { ASYNC_METHOD_MACRO( "RequestAuthorization",
                                                                GDBUS_ARGS( { "device", "o" } ),
                                                                nullptr,
                                                                AgentManager::Agent,
                                                                requestAuthorization ) },
                                          { ASYNC_METHOD_MACRO( "AuthorizeService",
                                                                GDBUS_ARGS( { "device", "o" }, { "uuid", "s" } ),
                                                                nullptr,
                                                                AgentManager::Agent,
                                                                authorizeService ) },
                                          { METHOD_MACRO( "Cancel", nullptr, nullptr, AgentManager::Agent, cancel ) },
                                          {} };

}  // namespace

AgentManager::Agent::Agent( const std::string& path )
: m_path( path ) {}

DBusMessage* AgentManager::Agent::release( DBusConnection* connection, DBusMessage* message ) {
    g_dbus_unregister_interface( connection, m_path.c_str(), BluezInterfaces::AgentManager );

    return dbus_message_new_method_return( message );
}

DBusMessage* AgentManager::Agent::requestPinCode( DBusConnection* connection, DBusMessage* message ) {
    LOG_INFO( "Agent::requestPinCode is called" );
    return dbus_message_new_method_return( message );
}

DBusMessage* AgentManager::Agent::displayPinCode( DBusConnection* connection, DBusMessage* message ) {
    LOG_INFO( "Agent::displayPinCode is called" );
    return dbus_message_new_method_return( message );
}

DBusMessage* AgentManager::Agent::requestPasskey( DBusConnection* connection, DBusMessage* message ) {
    LOG_INFO( "Agent::requestPasskey is called" );
    return dbus_message_new_method_return( message );
}

DBusMessage* AgentManager::Agent::displayPasskey( DBusConnection* connection, DBusMessage* message ) {
    LOG_INFO( "Agent::displayPasskey is called" );
    return dbus_message_new_method_return( message );
}

DBusMessage* AgentManager::Agent::requestConfirmation( DBusConnection* connection, DBusMessage* message ) {
    LOG_INFO( "Agent::requestConfirmation is called" );
    return dbus_message_new_method_return( message );
}

DBusMessage* AgentManager::Agent::requestAuthorization( DBusConnection* connection, DBusMessage* message ) {
    LOG_INFO( "Agent::requestAuthorization is called" );
    return dbus_message_new_method_return( message );
}

DBusMessage* AgentManager::Agent::authorizeService( DBusConnection* connection, DBusMessage* message ) {
    LOG_INFO( "Agent::authorizeService is called" );
    return dbus_message_new_method_return( message );
}

DBusMessage* AgentManager::Agent::cancel( DBusConnection* connection, DBusMessage* message ) {
    LOG_INFO( "Agent::cancel is called" );
    return dbus_message_new_method_return( message );
}

AgentManager::AgentManager( DBusConnection* connection, const AgentManagerProxy& proxy )
: m_connection( connection )
, m_agentManagerProxy( proxy ) {}

void AgentManager::registerAgent( AgentCapability capability ) {
    if( m_registered ) {
        return;
    }

    auto agent = std::make_unique< Agent >( m_agentManagerObjetPath );

    if( g_dbus_register_interface( m_connection,
                                   m_agentManagerObjetPath.c_str(),
                                   BluezInterfaces::AgentManager,
                                   AgentMethods,
                                   nullptr,
                                   nullptr,
                                   agent.get(),
                                   nullptr ) == false ) {
        LOG_ERROR( "Failed to register an agent" );

        return;
    }

    m_agentManagerProxy.registerAgent( m_agentManagerObjetPath, getString( capability ) );
    m_agentManagerProxy.requestDefaultAgent( m_agentManagerObjetPath );

    m_agent = std::move( agent );
    m_registered = true;

    LOG_INFO( "Agent is registered" );
}

void AgentManager::unregisterAgent() {
    if( !m_registered ) {
        return;
    }

    m_agentManagerProxy.unregisterAgent( m_agentManagerObjetPath );

    g_dbus_unregister_interface( m_connection, m_agentManagerObjetPath.c_str(), BluezInterfaces::AgentManager );

    m_agent.reset();
    m_registered = false;

    LOG_INFO( "Agent is unregistered" );
}

}  // namespace HubPlatform
