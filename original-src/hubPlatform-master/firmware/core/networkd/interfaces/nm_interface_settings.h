#pragma once

#include "networkd/interfaces/nm_enable_policy.h"

#include "database_client/idbclient.h"

namespace HubPlatform {
namespace Network {

class InterfaceSettings {
public:
    InterfaceSettings( IDbClient& dbClient, std::string interfaceName, EnablePolicy defaultEnablePolicy );

    const std::string& getInterfaceName() const;

    EnablePolicy getEnablePolicy() const;
    bool setEnablePolicy( EnablePolicy policy );

    void reset();

protected:
    Json getCustomSettings() const;
    void setCustomSettings( Json customSettings );

private:
    void loadSettings( EnablePolicy defaultEnablePolicy );
    void saveSettings();

private:
    IDbClient& m_dbClient;
    std::string m_interfaceName;
    EnablePolicy m_defaultEnablePolicy;

private:
    EnablePolicy m_enablePolicy;
    Json m_customSettings;
};

}  // namespace Network
}  // namespace HubPlatform
