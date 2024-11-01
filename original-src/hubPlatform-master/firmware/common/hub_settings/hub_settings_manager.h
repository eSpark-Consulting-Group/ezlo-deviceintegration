#pragma once

#include "config_reader/config_reader.h"
#include "database_client/idbclient.h"

namespace HubPlatform {
namespace Hub {

class SettingsManager final {
public:
    SettingsManager( IDbClient& dbClient, const ConfigReader& configReader );

    bool setSettingValue( const std::string& name, const Json& value );

private:
    IDbClient& m_dbClient;
    const ConfigReader& m_configReader;
};

}  // namespace Hub
}  // namespace HubPlatform
