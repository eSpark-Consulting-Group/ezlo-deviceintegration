#pragma once
#include "database_client/idbclient.h"
#include "hub_settings/hub_setting.h"

namespace HubPlatform {
namespace Hub {

class SettingsReader final {
public:
    explicit SettingsReader( const IDbClient& dbClient );

    std::vector< Setting > getSettings() const;

    using OnSettingFound = std::function< void( Setting&& ) >;
    int getSettings( const OnSettingFound& callback ) const;

    std::unique_ptr< Setting > findSetting( const std::string& setting ) const;

private:
    const IDbClient& m_dbClient;
};

}  // namespace Hub
}  // namespace HubPlatform
