#pragma once

#include "json_utils/json_utils.h"
#include "log_utils/ezlo_log.h"
#include <string>
#include <set>

class IDbClient;

namespace HubPlatform {
namespace CoreD {
namespace SwingerShutdown {

class SwingerShutdownSettings {
public:
    SwingerShutdownSettings( const Json& record );

public:
    int getDefaultHitsLimit();
    int64_t getDefaultInactiveWindow();
    std::list< std::string > getDisabledDevices();
    std::set< std::string > getDevicesIds();
    void addDisabledDevice( const std::string& deviceId );
    void removeDisabledDevice( const std::string& deviceId );
    bool isDisabledDevice( const std::string& deviceId );
    JsonObject getDeviceLimits( const std::string& deviceId );
    void resetDeviceLimits( const std::string& deviceId );

    void setDeviceLimits( const std::string& deviceId, int hitsLimit, int64_t inactiveWindow );

    Json toJson() const;
    bool fromJson( const JsonObject& data );

    static Json loadFromDb( const std::string& dbName, IDbClient& dbClient );
    static void createDbRecord( JsonObject query, const std::string dbName, IDbClient& dbClient );
    static void updateDbRecord( JsonObject updateObj, const std::string& dbName, IDbClient& dbClient );

private:
    int m_hitsLimitDefault = 0;
    int m_inactiveWindowDefault = 0;
    std::list< std::string > m_disabledDevices;
    std::string m_version;
    std::map< std::string, JsonObject > m_devices;
};

}  // namespace SwingerShutdown
}  // namespace CoreD
}  // namespace HubPlatform