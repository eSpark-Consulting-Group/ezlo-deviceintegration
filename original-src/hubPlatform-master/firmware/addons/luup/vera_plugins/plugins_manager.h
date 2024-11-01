#pragma once
#include <string>

class IDbClient;

class PluginsManager final {
public:
    PluginsManager( IDbClient& dbClient );

    bool installPlugin( const std::string& pluginId,
                        const std::string& version,
                        const std::string& aesKey,
                        const std::string& stateVariables );

    bool removePlugin( const std::string& pluginId );

private:
    IDbClient& m_dbClient;
};
