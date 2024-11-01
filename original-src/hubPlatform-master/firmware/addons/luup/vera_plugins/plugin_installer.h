#pragma once
#include <string>

class IDbClient;

// temporary
extern const std::string pluginsDbName;

class PluginInstaller final {
public:
    PluginInstaller( IDbClient& dbClient, std::string pluginId );

    bool createDevices();
    bool createServices();
    //...

private:
    IDbClient& m_dbClient;
    std::string m_pluginId;
};
