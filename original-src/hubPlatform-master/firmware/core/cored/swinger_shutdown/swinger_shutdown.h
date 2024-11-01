#pragma once

#include "cored/remote_interface/cored_interface.h"
#include "service_interfaces/postponed_task.h"
#include "config_reader/config_reader.h"
#include "swinger_shutdown_cleaner.h"
#include "resources.h"
#include "swinger_shutdown_settings.h"
#include <string>
#include <chrono>

class IDbClient;

namespace HubPlatform {
namespace CoreD {
namespace SwingerShutdown {

class SwingerShutdown {
public:
    SwingerShutdown( Services::Service& service, const ConfigReader& configReader, int syncPeriod = 1800 );
    explicit SwingerShutdown( Services::Service& service, int syncPeriod = 1800 );

    bool addHit( const std::string& id, int timeFrame = Resources::TimeFrameSeconds );
    void addDeviceHit( const std::string& id );
    void setDeviceDeleted( const std::string& id, bool deleted );
    void setDeletedForTimeSeries( const std::string& id, bool deleted );
    void syncWithDeviceStatisticPersistentStore();
    void syncWithTimeSeriesPersistentStore();
    void initDefaults();
    JsonObject getConfigLimitsDefault( const ConfigReader& configReader );
    void addDeviceHitToTimeSeries( const std::string& id, int timeFrame = Resources::TimeFrameSeconds );
    int64_t calculateBeginOfIntervalUtc( const std::string& id, int64_t timePointInt, int timeFrame );
    void syncTask();
    void clean();
    void markDeviceRecordsAsDeleted( const std::string& id );

    JsonObject list( const Json& record );
    JsonObject swingerShutdownReset( const Json& record );
    JsonObject swingerShutdownDisableAdd( const Json& record );
    JsonObject swingerShutdownDisableRemove( const Json& record );
    JsonObject swingerShutdownLimitsSet( const Json& record );
    JsonObject swingerShutdownLimitsReset( const Json& request );

    bool isReachedDeviceHitsLimit( const std::string& deviceId );
    bool isDisabled( const std::string& deviceId );
    bool isExist( const std::string& deviceId );
    int getHits( const std::string& deviceId ) const;

private:
    IDbClient& getDBClient();
    Json createTempStatisticRecord( const std::string& id ) const;
    Json createTempTimeSeriesRecord( const std::string& id ) const;
    uint64_t getBeginBoundary( const std::string& id, const std::string& storage );
    void markStatisticRecordAsDeleted( const std::string& id, const std::string& dbName );
    void markTimeSeriesRecordsAsDeleted( const std::string& id, const std::string& dbName );
    JsonArrayBuilder getDeviceInfo( HubPlatform::CoreD::SwingerShutdown::SwingerShutdownSettings& settings,
                                    const std::set< std::string >& ids ) const;
    std::set< std::string > getDeviceIds() const;
    bool isTheSameVersion( const Json& record, const std::string& currentVersion ) const;
    Json getStatInfo( const SwingerShutdownDeviceStatistic& statistic ) const;
    JsonArrayBuilder getDisabledDevicesInfo( SwingerShutdownSettings& settings ) const;
    JsonObject getDefaultsLimitInfo( SwingerShutdownSettings& settings ) const;
    void emitEvent( MessageSubClasses msgClass, const Json& payload ) const;
    int getDeviceHitsLimit( const std::string& deviceId ) const;
    bool isArmedFieldPresentInDevice( const std::string& deviceId );

private:
    IDbClient& m_dbClient;
    Services::Service& m_service;
    Services::PostponedTaskPtr m_syncPersistentDBTask;
    SwingerShutdownCleanerPtr m_cleaner;
    int m_hitsLimit = 0;
    int m_inactivityWindow = 0;
    std::map< std::string, Json > m_limits;
};

}  // namespace SwingerShutdown
}  // namespace CoreD
}  // namespace HubPlatform