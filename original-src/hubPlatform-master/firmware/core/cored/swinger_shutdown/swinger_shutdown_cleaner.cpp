#include "swinger_shutdown_cleaner.h"
#include "swinger_shutdown_devices_statistic.h"
#include "swinger_shutdown_settings.h"
#include "service_interfaces/service.h"
#include "json_utils/json_utils.h"
#include "log_utils/ezlo_log.h"
#include "database_client/dbid.h"
#include "database_client/dbqueryparser.h"
#include "database_client/idbclient.h"
#include "resources.h"

namespace HubPlatform {
namespace CoreD {
namespace SwingerShutdown {

static constexpr std::chrono::minutes CheckPeriod{ 1 };

using namespace Resources;

IDbClient& SwingerShutdownCleaner::getDBClient() {
    return m_dbClient;
}

SwingerShutdownCleaner::SwingerShutdownCleaner( Services::Service& service,
                                                int hitsLimit,
                                                int inactivityWindow,
                                                int periodBeforeDelete )
: m_dbClient( service.getDBClient() )
, m_service( service )
, m_hitsLimitDefault( hitsLimit )
, m_inactivityWindowDefault( inactivityWindow )
, m_periodBeforeDelete( periodBeforeDelete ) {
    m_checkRecordsTask = m_service.createPostponedTask( [this]( Services::PostponedTask& ) { run(); } );
    m_checkRecordsTask->start( CheckPeriod, true );
}

std::chrono::seconds SwingerShutdownCleaner::now() {
    return std::chrono::duration_cast< std::chrono::seconds >( std::chrono::system_clock::now().time_since_epoch() );
}

void SwingerShutdownCleaner::run() {
    checkTimeSeriesAndProcess();
    updateSummaryInfo();
}

void SwingerShutdownCleaner::checkTimeSeriesAndProcess() {
    auto seriesPersistentDB = DBTables::PersistentTimeSeriesDB;
    auto allRecords = SwingerShutdownTimeSeries::loadAllRecordsFromDb(
        seriesPersistentDB, JSON( Resources::TimeSeriesFields::Deleted << false ), m_dbClient );
    for( JsonObject& record : allRecords ) {
        SwingerShutdownTimeSeries series( record );
        checkAndMarkAsDeletedSeries( series );
    }

    allRecords = SwingerShutdownTimeSeries::loadAllRecordsFromDb(
        seriesPersistentDB, JSON( Resources::TimeSeriesFields::Deleted << true ), m_dbClient );
    for( JsonObject& record : allRecords ) {
        SwingerShutdownTimeSeries series( record );
        auto beginTime = std::chrono::seconds( series.getBegin() );
        checkAndDeleteSeries( beginTime, series );
    }
}

JsonObject SwingerShutdownCleaner::getLimitsForDevice( const std::string& deviceId ) const {
    JsonObject data = SwingerShutdownSettings::loadFromDb( HouseModesSwingerShutdown, m_dbClient );
    SwingerShutdownSettings settings( data );
    JsonObject limits = settings.getDeviceLimits( deviceId );
    return limits;
}

std::chrono::seconds SwingerShutdownCleaner::getInactivityWindow( const std::string& deviceId ) {
    JsonObject limits = getLimitsForDevice( deviceId );
    auto inactivityWindow = json_get_int( limits, Settings::InactivityWindow );

    if( inactivityWindow > 0 ) {
        return std::chrono::seconds( inactivityWindow );
    }

    return m_inactivityWindowDefault;
}

int SwingerShutdownCleaner::getHitsLimit( const std::string& deviceId ) {
    JsonObject limits = getLimitsForDevice( deviceId );
    int hitsLimit = json_get_int( limits, Settings::HitsLimit );

    if( hitsLimit > 0 ) {
        return hitsLimit;
    }

    return m_hitsLimitDefault;
}

void SwingerShutdownCleaner::checkAndMarkAsDeletedSeries( SwingerShutdownTimeSeries& series ) {
    auto deviceId = series.getDeviceId();
    auto beginTime = std::chrono::seconds( series.getBegin() );

    auto inactivityWindowDefault = getInactivityWindow( deviceId );

    if( now() - beginTime >= inactivityWindowDefault ) {
        series.setDeleted( true );
    }
    SwingerShutdownTimeSeries::updateDbRecord(
        series.getId(), series.toJson(), DBTables::PersistentTimeSeriesDB, m_dbClient );
}

void SwingerShutdownCleaner::checkAndDeleteSeries( std::chrono::seconds beginTime, SwingerShutdownTimeSeries& series ) {
    auto deviceId = series.getDeviceId();

    auto inactivityWindowDefault = getInactivityWindow( deviceId );

    if( now() - beginTime >= inactivityWindowDefault + m_periodBeforeDelete ) {
        series.deleteDbRecord( series.getId(), DBTables::PersistentTimeSeriesDB, m_dbClient );
    }
}

void SwingerShutdownCleaner::updateSummaryInfo() {
    auto fields = JSON( "_id" << true );
    auto request = JSON_EMPTY_OBJ;
    auto allRecords =
        SwingerShutdownDeviceStatistic::loadAllFromDb( DBTables::PersistentStatisticDB, request, &fields, m_dbClient );
    for( auto& idRecord : allRecords ) {
        SwingerShutdownDeviceStatistic summaryId( idRecord );
        auto id = summaryId.getId();
        int activeHitsCount = getActiveHitsInTimeSeries( id );
        auto record = SwingerShutdownDeviceStatistic::loadFromDb( id, DBTables::PersistentStatisticDB, m_dbClient );
        SwingerShutdownDeviceStatistic summaryRecord( record );
        summaryRecord.setHits( activeHitsCount );
        checkAndRemoveDeletedDevicesFromSummary( summaryRecord );
        SwingerShutdownDeviceStatistic::updateDbRecord(
            id, summaryRecord.toJson(), DBTables::PersistentStatisticDB, m_dbClient );
        record = SwingerShutdownDeviceStatistic::loadFromDb( id, DBTables::TempStatisticDB, m_dbClient );
        if( !record.empty() ) {
            SwingerShutdownDeviceStatistic summaryRecordTemp( record );
            summaryRecordTemp.setHits( activeHitsCount );
            checkAndRemoveDeletedDevicesFromSummary( summaryRecordTemp );
            SwingerShutdownDeviceStatistic::updateDbRecord(
                id, summaryRecordTemp.toJson(), DBTables::TempStatisticDB, m_dbClient );
        }
    }
}

void SwingerShutdownCleaner::checkAndRemoveDeletedDevicesFromSummary( SwingerShutdownDeviceStatistic& summaryRecord ) {
    bool isSummaryDeletedRecord = summaryRecord.getDeleted();
    if( isSummaryDeletedRecord ) {
        SwingerShutdownDeviceStatistic::deleteDbRecord(
            summaryRecord.getId(), DBTables::PersistentStatisticDB, m_dbClient );
    }
}

int SwingerShutdownCleaner::getActiveHitsInTimeSeries( const std::string& deviceId ) {
    auto allTimeSeriesRecordsForId = m_dbClient.findAll(
        DBTables::PersistentTimeSeriesDB,
        JSON( Resources::TimeSeriesFields::DeviceId << deviceId << Resources::TimeSeriesFields::Deleted << false ),
        nullptr );
    int activeHitsCount = 0;
    for( auto record : allTimeSeriesRecordsForId ) {
        SwingerShutdownTimeSeries timeSeries( record );
        activeHitsCount += timeSeries.getHits();
    }
    return activeHitsCount;
}

}  // namespace SwingerShutdown
}  // namespace CoreD
}  // namespace HubPlatform
