#pragma once

#include "cored/remote_interface/cored_interface.h"
#include "swinger_shutdown_time_series.h"
#include "swinger_shutdown_devices_statistic.h"
#include "service_interfaces/postponed_task.h"
#include "config_reader/config_reader.h"
#include "resources.h"
#include <string>
#include <chrono>

class IDbClient;

namespace HubPlatform {

namespace UT {
class SwingerShutdownTestSuite;
}  // namespace UT

namespace CoreD {
namespace SwingerShutdown {

class SwingerShutdownCleaner;

using SwingerShutdownCleanerPtr = std::unique_ptr< SwingerShutdownCleaner >;

class SwingerShutdownCleaner {
public:
    SwingerShutdownCleaner( Services::Service& service, int hitsLimit, int inactivityWindow, int periodBeforeDelete );

public:
    void run();
    void checkTimeSeriesAndProcess();

private:
    IDbClient& getDBClient();
    static std::chrono::seconds now();
    void checkAndDeleteSeries( std::chrono::seconds beginTime, SwingerShutdownTimeSeries& series );
    void checkAndMarkAsDeletedSeries( SwingerShutdownTimeSeries& series );
    void updateSummaryInfo();
    int getActiveHitsInTimeSeries( const std::string& id );
    void checkAndRemoveDeletedDevicesFromSummary( SwingerShutdownDeviceStatistic& summaryRecord );
    std::chrono::seconds getInactivityWindow( const std::string& deviceId );
    int getHitsLimit( const std::string& deviceId );

    friend class HubPlatform::UT::SwingerShutdownTestSuite;

private:
    IDbClient& m_dbClient;
    Services::Service& m_service;
    int m_hitsLimitDefault = 0;
    std::chrono::seconds m_inactivityWindowDefault = std::chrono::seconds( 0 );
    Services::PostponedTaskPtr m_checkRecordsTask;
    std::chrono::seconds m_periodBeforeDelete = std::chrono::seconds( Resources::PeriodBeforeDeleteSeriesSeconds );

    JsonObject getLimitsForDevice( const std::string& deviceId ) const;
};

}  // namespace SwingerShutdown
}  // namespace CoreD
}  // namespace HubPlatform