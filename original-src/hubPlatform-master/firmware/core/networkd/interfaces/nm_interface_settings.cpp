#include "networkd/interfaces/nm_interface_settings.h"

#include "database_client/dbqueryparser.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace Network {

namespace Keys {
constexpr char EnablePolicy[] = "enabled";
constexpr char CustomSettings[] = "customSettings";
}  // namespace Keys

InterfaceSettings::InterfaceSettings( IDbClient& dbClient, std::string interfaceName, EnablePolicy defaultEnablePolicy )
: m_dbClient( dbClient )
, m_interfaceName( std::move( interfaceName ) )
, m_defaultEnablePolicy( defaultEnablePolicy ) {
    loadSettings( m_defaultEnablePolicy );
}

const std::string& InterfaceSettings::getInterfaceName() const {
    return m_interfaceName;
}

EnablePolicy InterfaceSettings::getEnablePolicy() const {
    return m_enablePolicy;
}

bool InterfaceSettings::setEnablePolicy( EnablePolicy policy ) {
    if( m_enablePolicy == policy )
        return false;

    m_enablePolicy = policy;
    saveSettings();
    return true;
}

void InterfaceSettings::reset() {
    m_dbClient.remove( InterfaceSettingsDBName, JSON( db::ID_FIELD_NAME << getInterfaceName() ) );
    loadSettings( m_defaultEnablePolicy );
}

Json InterfaceSettings::getCustomSettings() const {
    return m_customSettings;
}

void InterfaceSettings::setCustomSettings( Json customSettings ) {
    m_customSettings = std::move( customSettings );
    saveSettings();
}

void InterfaceSettings::loadSettings( EnablePolicy defaultEnablePolicy ) {
    auto settings =
        m_dbClient.findOne( InterfaceSettingsDBName, JSON( db::ID_FIELD_NAME << getInterfaceName() ), nullptr );

    m_enablePolicy = enablePolicyFromString( json_get_str( settings, Keys::EnablePolicy ), defaultEnablePolicy );
    m_customSettings = settings.count( Keys::CustomSettings ) > 0 ? settings[ Keys::CustomSettings ] : JSON_EMPTY_OBJ;
}

void InterfaceSettings::saveSettings() {
    Json settings;
    settings[ db::ID_FIELD_NAME ] = getInterfaceName();
    settings[ Keys::EnablePolicy ] = toString( m_enablePolicy );
    settings[ Keys::CustomSettings ] = m_customSettings;
    m_dbClient.update( InterfaceSettingsDBName, JSON_EMPTY_OBJ, settings, true, false );
}

}  // namespace Network
}  // namespace HubPlatform
