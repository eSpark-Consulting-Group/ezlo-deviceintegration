#pragma once

#include "json_utils/json_fwd.h"

class IDbClient;
namespace HubPlatform {
namespace Network {

class StatisticsSettingsManager final {
public:
    struct StatisticsSettings {
        StatisticsSettings();
        explicit StatisticsSettings( const Json& settings );

        int m_storageSize = -1;  // in days

        Json toJson() const;
        bool empty() const;
    };

    StatisticsSettingsManager( IDbClient& );

    void setSetting( const StatisticsSettings& settings );
    StatisticsSettings getSetting() const;

private:
    IDbClient& m_dbClient;
};

}  // namespace Network
}  // namespace HubPlatform
