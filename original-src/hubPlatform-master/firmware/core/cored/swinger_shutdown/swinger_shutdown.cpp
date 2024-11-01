#include "swinger_shutdown.h"
#include "swinger_shutdown_devices_statistic.h"
#include "swinger_shutdown_time_series.h"
#include "swinger_shutdown_settings.h"
#include "service_interfaces/service.h"
#include "json_utils/json_utils.h"
#include "log_utils/ezlo_log.h"
#include "database_client/dbid.h"
#include "database_client/dbqueryparser.h"
#include "database_client/idbclient.h"
#include "database_client/db_tables_names.h"
#include "resources.h"
#include "config_reader/config_keys.h"
#include "common_utils/formatters.h"
#include "base_utils/ezlodb.h"
#include "common_utils/cpp14.h"
#include "swinger_shutdown_cleaner.h"
#include "crypto_utils/digests.h"
#include "cored/remote_interface/cored_interface_factory.h"

namespace HubPlatform {
namespace CoreD {
namespace SwingerShutdown {

using namespace Resources;

IDbClient& SwingerShutdown::getDBClient() {
    return m_dbClient;
}

SwingerShutdown::SwingerShutdown( Services::Service& service, const ConfigReader& configReader, int syncPeriod )
: m_dbClient( service.getDBClient() )
, m_service( service ) {
    getConfigLimitsDefault( configReader );
    initDefaults();
    m_syncPersistentDBTask = m_service.createPostponedTask( [this]( Services::PostponedTask& ) { syncTask(); } );

    m_syncPersistentDBTask->start( std::chrono::seconds( syncPeriod ), true );

    m_cleaner = std::make_unique< SwingerShutdownCleaner >(
        m_service, m_hitsLimit, m_inactivityWindow, Resources::PeriodBeforeDeleteSeriesSeconds );
}

SwingerShutdown::SwingerShutdown( Services::Service& service, int syncPeriod )
: m_dbClient( service.getDBClient() )
, m_service( service ) {
    initDefaults();
    m_syncPersistentDBTask = m_service.createPostponedTask( [this]( Services::PostponedTask& ) { syncTask(); } );

    m_syncPersistentDBTask->start( std::chrono::seconds( syncPeriod ), true );

    m_cleaner = std::make_unique< SwingerShutdownCleaner >(
        m_service, m_hitsLimit, m_inactivityWindow, Resources::PeriodBeforeDeleteSeriesSeconds );
}

void SwingerShutdown::syncTask() {
    syncWithDeviceStatisticPersistentStore();
    syncWithTimeSeriesPersistentStore();
}

bool SwingerShutdown::addHit( const std::string& id, int timeFrame ) {
    auto record = SwingerShutdownDeviceStatistic::loadFromDb( id, DBTables::PersistentStatisticDB, m_dbClient );
    if( !record.empty() ) {
        SwingerShutdownDeviceStatistic shutdownDeviceInfo( record );
        if( shutdownDeviceInfo.getDeleted() == false ) {
            addDeviceHit( id );
            addDeviceHitToTimeSeries( id, timeFrame );
        }
        else {
            return false;
        }
    }
    else {
        addDeviceHit( id );
        addDeviceHitToTimeSeries( id, timeFrame );
    }

    return true;
}

void SwingerShutdown::addDeviceHit( const std::string& id ) {
    auto record = SwingerShutdownDeviceStatistic::loadFromDb( id, DBTables::TempStatisticDB, m_dbClient );
    if( record.empty() ) {
        record = createTempStatisticRecord( id );
    }
    SwingerShutdownDeviceStatistic shutdownDeviceInfo( record );
    auto hits = shutdownDeviceInfo.getHits();
    shutdownDeviceInfo.setHits( hits + 1 );
    auto now = std::chrono::duration_cast< std::chrono::seconds >( std::chrono::system_clock::now().time_since_epoch() )
                   .count();
    if( hits == 0 ) {
        shutdownDeviceInfo.setAdded( now );
        shutdownDeviceInfo.setUpdated( now );
    }
    else {
        shutdownDeviceInfo.setUpdated( now );
    }
    SwingerShutdownDeviceStatistic::updateDbRecord(
        id, shutdownDeviceInfo.toJson(), DBTables::TempStatisticDB, m_dbClient );
}

std::chrono::system_clock::time_point addPeriod( const std::chrono::system_clock::time_point& timePoint, int seconds ) {
    return timePoint + std::chrono::seconds( seconds );
}

int64_t convertTimePointToSecondsNumber( const std::chrono::system_clock::time_point& timePoint ) {
    auto convertedTime = timePoint.time_since_epoch();
    return std::chrono::duration_cast< std::chrono::seconds >( convertedTime ).count();
}

int64_t calculateBoundaryForTimePoint( const std::chrono::system_clock::time_point& timePoint,
                                       int timeFrame,
                                       int64_t begin ) {
    auto currentBoundary = std::chrono::system_clock::from_time_t( static_cast< time_t >( begin ) );
    while( true ) {
        auto nextBoundary = addPeriod( currentBoundary, timeFrame );
        if( timePoint < nextBoundary ) {
            return convertTimePointToSecondsNumber( currentBoundary );
        }
        currentBoundary = nextBoundary;
    }
}

uint64_t SwingerShutdown::getBeginBoundary( const std::string& id, const std::string& storage ) {
    auto record = SwingerShutdownTimeSeries::loadFromDb( id, storage, m_dbClient );
    int64_t begin = 0;
    if( !record.empty() ) {
        SwingerShutdownTimeSeries timeSeries( record );
        begin = timeSeries.getBegin();
    }
    return begin;
}

int64_t SwingerShutdown::calculateBeginOfIntervalUtc( const std::string& id, int64_t timePointInt, int timeFrame ) {
    auto timePoint = std::chrono::system_clock::from_time_t( timePointInt );
    auto begin = getBeginBoundary( id, DBTables::TempTimeSeriesDB );
    if( begin == 0 ) {
        begin = getBeginBoundary( id, DBTables::PersistentTimeSeriesDB );
    }

    if( begin == 0 ) {
        return convertTimePointToSecondsNumber( timePoint );
    }

    return calculateBoundaryForTimePoint( timePoint, timeFrame, begin );
}

void SwingerShutdown::addDeviceHitToTimeSeries( const std::string& id, int timeFrame ) {
    auto storage = DBTables::TempTimeSeriesDB;
    auto record = SwingerShutdownTimeSeries::loadFromDb( id, storage, m_dbClient );
    if( record.empty() ) {
        record = createTempTimeSeriesRecord( id );
    }
    SwingerShutdownTimeSeries timeSeries( record );
    timeSeries.setDeviceId( id );
    auto hits = timeSeries.getHits();
    int64_t begin = timeSeries.getBegin();
    timeSeries.setHits( hits + 1 );
    auto now = std::chrono::duration_cast< std::chrono::seconds >( std::chrono::system_clock::now().time_since_epoch() )
                   .count();
    auto calculatedBegin = calculateBeginOfIntervalUtc( id, now, timeFrame );

    if( calculatedBegin > begin ) {
        timeSeries.setBegin( calculatedBegin );
    }
    else {
        timeSeries.setBegin( begin );
    }

    timeSeries.setUpdated( now );

    SwingerShutdownTimeSeries::updateDbRecord(
        id, timeSeries.toJson(), TempFirmwareCoredSwingerShutdownTimeSeries, m_dbClient );
}

Json SwingerShutdown::createTempStatisticRecord( const std::string& id ) const {
    auto record = SwingerShutdownDeviceStatistic::loadFromDb( id, DBTables::PersistentStatisticDB, m_dbClient );
    if( !record.empty() ) {
        record = SwingerShutdownDeviceStatistic::loadFromDb( id, DBTables::PersistentStatisticDB, m_dbClient );
        SwingerShutdownDeviceStatistic shutdownDeviceInfoSaved( record );
        SwingerShutdownDeviceStatistic::createDbRecord( id, record, DBTables::TempStatisticDB, m_dbClient );
    }
    else {
        SwingerShutdownDeviceStatistic::createDbRecord( id, record, DBTables::TempStatisticDB, m_dbClient );
    }
    record = SwingerShutdownDeviceStatistic::loadFromDb( id, DBTables::TempStatisticDB, m_dbClient );
    return record;
}

Json SwingerShutdown::createTempTimeSeriesRecord( const std::string& id ) const {
    auto record = SwingerShutdownTimeSeries::loadFromDb( id, DBTables::PersistentTimeSeriesDB, m_dbClient );
    if( !record.empty() ) {
        record = SwingerShutdownTimeSeries::loadFromDb( id, DBTables::PersistentTimeSeriesDB, m_dbClient );
        SwingerShutdownTimeSeries shutdownDeviceInfoSaved( record );
        SwingerShutdownTimeSeries::createDbRecord( id, record, DBTables::TempTimeSeriesDB, m_dbClient );
    }
    else {
        SwingerShutdownTimeSeries::createDbRecord( id, record, DBTables::TempTimeSeriesDB, m_dbClient );
    }
    record = SwingerShutdownTimeSeries::loadFromDb( id, DBTables::TempTimeSeriesDB, m_dbClient );
    return record;
}

void SwingerShutdown::setDeviceDeleted( const std::string& id, bool deleted ) {
    auto record = SwingerShutdownDeviceStatistic::loadFromDb( id, DBTables::TempStatisticDB, m_dbClient );
    if( record.empty() ) {
        record = createTempStatisticRecord( id );
    }
    SwingerShutdownDeviceStatistic shutdownDeviceInfo( record );
    SwingerShutdownDeviceStatistic::loadFromDb( id, DBTables::TempStatisticDB, m_dbClient );
    shutdownDeviceInfo.setDeleted( deleted );
    SwingerShutdownDeviceStatistic::updateDbRecord(
        id, shutdownDeviceInfo.toJson(), DBTables::TempStatisticDB, m_dbClient );
}

void SwingerShutdown::setDeletedForTimeSeries( const std::string& id, bool deleted ) {
    auto record = SwingerShutdownTimeSeries::loadFromDb( id, DBTables::TempTimeSeriesDB, m_dbClient );
    if( record.empty() ) {
        createTempTimeSeriesRecord( id );
    }
    SwingerShutdownTimeSeries info( record );
    SwingerShutdownTimeSeries::loadFromDb( id, DBTables::TempTimeSeriesDB, m_dbClient );
    info.setDeleted( deleted );
    SwingerShutdownTimeSeries::updateDbRecord( id, info.toJson(), DBTables::TempTimeSeriesDB, m_dbClient );
}

void SwingerShutdown::syncWithDeviceStatisticPersistentStore() {
    auto temporaryData = m_dbClient.findAll( DBTables::TempStatisticDB, JSON_EMPTY_OBJ, nullptr );
    for( JsonObject& tempRecord : temporaryData ) {
        SwingerShutdownDeviceStatistic statisticTemp( tempRecord );
        auto id = statisticTemp.getId();
        auto storedRecord = m_dbClient.findOne(
            DBTables::PersistentStatisticDB, JSON( Resources::StatisticFields::Id << id ), nullptr );
        if( storedRecord.empty() ) {
            SwingerShutdownDeviceStatistic::createDbRecord(
                id, statisticTemp.toJson(), DBTables::PersistentStatisticDB, m_dbClient );
        }
        else {
            JsonObject persistentRecord =
                SwingerShutdownDeviceStatistic::loadFromDb( id, DBTables::PersistentStatisticDB, m_dbClient );
            SwingerShutdownDeviceStatistic statistic( persistentRecord );
            int storedHits = statistic.getHits();
            auto tempHits = statisticTemp.getHits();
            statisticTemp.setHits( tempHits + storedHits );
            SwingerShutdownDeviceStatistic::updateDbRecord(
                id, statisticTemp.toJson(), DBTables::PersistentStatisticDB, m_dbClient );
        }
    }
}

void SwingerShutdown::syncWithTimeSeriesPersistentStore() {
    auto temporaryData = m_dbClient.findAll( DBTables::TempTimeSeriesDB, JSON_EMPTY_OBJ, nullptr );
    for( JsonObject& record : temporaryData ) {
        auto deviceId = json_get_str( record, Resources::TimeSeriesFields::DeviceId );
        JsonArray storedRecords = m_dbClient.findAll(
            DBTables::PersistentTimeSeriesDB, JSON( Resources::TimeSeriesFields::DeviceId << deviceId ), nullptr );
        auto id = Db::genOid();
        record.erase( Resources::TimeSeriesFields::Id );
        SwingerShutdownTimeSeries::createDbRecord( id, record, DBTables::PersistentTimeSeriesDB, m_dbClient );
        if( !storedRecords.empty() ) {
            for( auto& record : storedRecords ) {
                SwingerShutdownTimeSeries::createDbRecord( id, record, DBTables::PersistentTimeSeriesDB, m_dbClient );
            }
        }
    }

    if( !temporaryData.empty() ) {
        m_dbClient.clear( DBTables::TempTimeSeriesDB );
    }
}

JsonObject SwingerShutdown::getConfigLimitsDefault( const ConfigReader& configReader ) {
    Json defaults;
    using namespace Resources::ConfigFields;

    try {
        ConfigReader::Path path = ConfigReader::Path( configReader, ConfigKeys::SwingerShutdownDefaults );
        m_hitsLimit = configReader.getInt( path / "hitsLimit" );
        std::string inactivityWindow = configReader.getString( path / "inactivityWindow" );
        if( !inactivityWindow.empty() ) {
            uint64_t time;
            if( CommonUtils::timeFormatter( inactivityWindow, time ) ) {
                m_inactivityWindow = time / 1000000000;
            }
            else {
                LOGGER_ERROR( "Swinger Shutdown inactivityWindow parameter has incorrect value" );
            }
        }

        return JSON( "defaults" << JSON( HitsLimit << m_hitsLimit << InactivityWindow << m_inactivityWindow ) );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Cannot read config for SwingerShutdown reason: " ) + ex.what() );
    }

    return JSON_EMPTY_OBJ;
}

void SwingerShutdown::initDefaults() {
    using namespace Resources::ConfigFields;
    JsonObject defaults = SwingerShutdownSettings::loadFromDb( HouseModesSwingerShutdown, m_dbClient );

    auto query = JSON( Defaults << JSON( HitsLimit << m_hitsLimit << InactivityWindow << m_inactivityWindow ) );

    if( defaults.empty() ) {
        SwingerShutdownSettings::createDbRecord( query, HouseModesSwingerShutdown, m_dbClient );
    }
    else {
        SwingerShutdownSettings::updateDbRecord( query, HouseModesSwingerShutdown, m_dbClient );
    }
}

void SwingerShutdown::clean() {
    m_cleaner->run();
}

void SwingerShutdown::markStatisticRecordAsDeleted( const std::string& id, const std::string& dbName ) {
    auto record = SwingerShutdownDeviceStatistic::loadFromDb( id, dbName, m_dbClient );
    SwingerShutdownDeviceStatistic statistic( record );
    statistic.setDeleted( true );
    SwingerShutdownDeviceStatistic::updateDbRecord( id, statistic.toJson(), dbName, m_dbClient );
}

void SwingerShutdown::markTimeSeriesRecordsAsDeleted( const std::string& id, const std::string& dbName ) {
    auto records = SwingerShutdownTimeSeries::loadAllRecordsFromDb(
        dbName, JSON( Resources::TimeSeriesFields::DeviceId << id ), m_dbClient );
    for( auto record : records ) {
        SwingerShutdownTimeSeries series( record );
        series.setDeleted( true );
        SwingerShutdownTimeSeries::updateDbRecord( series.getId(), series.toJson(), dbName, m_dbClient );
    }
}

void SwingerShutdown::markDeviceRecordsAsDeleted( const std::string& id ) {
    using namespace Resources::DBTables;

    markStatisticRecordAsDeleted( id, TempStatisticDB );
    markStatisticRecordAsDeleted( id, PersistentStatisticDB );
    markTimeSeriesRecordsAsDeleted( id, TempTimeSeriesDB );
    markTimeSeriesRecordsAsDeleted( id, PersistentTimeSeriesDB );
}

std::string calculateVersionHashFromObject( JsonObject& data ) {
    auto hash = CryptoUtils::Digests::sha1( data.dump() );
    return hash;
}

JsonObject SwingerShutdown::list( const Json& record ) {
    using namespace Resources::Settings;
    auto res = JSON_EMPTY_OBJ;
    JsonObject data = SwingerShutdownSettings::loadFromDb( HouseModesSwingerShutdown, m_dbClient );
    SwingerShutdownSettings settings( data );

    res[ Limits ] = getDefaultsLimitInfo( settings );
    res[ Devices ] = getDeviceInfo( settings, getDeviceIds() );
    res[ DisabledDevices ] = getDisabledDevicesInfo( settings );
    auto versionHash = calculateVersionHashFromObject( res );
    if( isTheSameVersion( record, versionHash ) ) {
        return JSON_EMPTY_OBJ;
    }
    res[ Version ] = versionHash;

    return res;
}

int SwingerShutdown::getDeviceHitsLimit( const std::string& deviceId ) const {
    JsonObject data = SwingerShutdownSettings::loadFromDb( HouseModesSwingerShutdown, m_dbClient );
    SwingerShutdownSettings settings( data );
    JsonObject limits = settings.getDeviceLimits( deviceId );

    int hitsLimit = json_get_int( limits, Settings::HitsLimit, 0 );
    if( hitsLimit == 0 ) {
        hitsLimit = settings.getDefaultHitsLimit();
    }

    return hitsLimit;
}

int SwingerShutdown::getHits( const std::string& deviceID ) const {
    auto record = SwingerShutdownDeviceStatistic::loadFromDb( deviceID, DBTables::TempStatisticDB, m_dbClient );
    SwingerShutdownDeviceStatistic info( record );
    return info.getHits();
}

bool SwingerShutdown::isDisabled( const std::string& deviceId ) {
    JsonObject data = SwingerShutdownSettings::loadFromDb( HouseModesSwingerShutdown, m_dbClient );
    SwingerShutdownSettings settings( data );
    return settings.isDisabledDevice( deviceId );
}

bool SwingerShutdown::isExist( const std::string& deviceId ) {
    JsonObject statisticRecord =
        SwingerShutdownDeviceStatistic::loadFromDb( deviceId, DBTables::TempStatisticDB, m_dbClient );

    if( statisticRecord.empty() ) {
        return false;
    }
    return true;
}

bool SwingerShutdown::isReachedDeviceHitsLimit( const std::string& deviceId ) {
    int hitsLimit = getDeviceHitsLimit( deviceId );
    int currentHits = getHits( deviceId );
    if( currentHits >= hitsLimit ) {
        return true;
    }

    return false;
}

JsonObject SwingerShutdown::getDefaultsLimitInfo( SwingerShutdownSettings& settings ) const {
    auto hitsLimit = settings.getDefaultHitsLimit();
    auto inactivityWindow = settings.getDefaultInactiveWindow();
    JsonObject limit =
        JSON( ConfigFields::HitsLimit << hitsLimit << ConfigFields::InactivityWindow << inactivityWindow );
    return limit;
}

JsonArrayBuilder SwingerShutdown::getDisabledDevicesInfo( SwingerShutdownSettings& settings ) const {
    auto disabledDevicesList = settings.getDisabledDevices();
    JsonArrayBuilder disabledDevicesArr;
    for( auto& device : disabledDevicesList ) {
        disabledDevicesArr << device;
    }
    return disabledDevicesArr;
}

bool SwingerShutdown::isTheSameVersion( const Json& record, const std::string& currentVersion ) const {
    bool sameVersion = false;
    if( record.find( Settings::Version ) != record.end() ) {
        auto version = json_get_str( record, Settings::Version );
        if( version == currentVersion ) {
            sameVersion = true;
        }
    }
    return sameVersion;
}

std::set< std::string > SwingerShutdown::getDeviceIds() const {
    auto fields = JSON( StatisticFields::Id << true );
    auto request = JSON( StatisticFields::Deleted << false );

    auto allSavedIds =
        SwingerShutdownDeviceStatistic::loadAllFromDb( DBTables::PersistentStatisticDB, request, &fields, m_dbClient );
    auto allTempIds =
        SwingerShutdownDeviceStatistic::loadAllFromDb( DBTables::TempStatisticDB, request, &fields, m_dbClient );

    std::set< std::string > ids;
    for( auto& id : allSavedIds ) {
        auto idStr = json_get_str( id, StatisticFields::Id );
        ids.insert( idStr );
    }

    for( auto& id : allTempIds ) {
        auto idStr = json_get_str( id, StatisticFields::Id );
        ids.insert( idStr );
    }
    return ids;
}

JsonArrayBuilder SwingerShutdown::getDeviceInfo( SwingerShutdownSettings& settings,
                                                 const std::set< std::string >& summaryIds ) const {
    auto ids = settings.getDevicesIds();

    ids.insert( summaryIds.begin(), summaryIds.end() );

    JsonArrayBuilder deviceInfoArray;
    for( auto& id : ids ) {
        Json res;
        res[ Settings::DeviceId ] = id;
        JsonObject statisticRecord =
            SwingerShutdownDeviceStatistic::loadFromDb( id, DBTables::TempStatisticDB, m_dbClient );

        if( !statisticRecord.empty() ) {
            SwingerShutdownDeviceStatistic statistic( statisticRecord );
            Json stat = getStatInfo( statistic );
            res[ Settings::Stat ] = stat;
        }
        Json limits = settings.getDeviceLimits( id );
        res[ Settings::Limits ] = limits;
        deviceInfoArray << res;
    }
    return deviceInfoArray;
}

Json SwingerShutdown::getStatInfo( const SwingerShutdownDeviceStatistic& statistic ) const {
    int hits = statistic.getHits();
    int64_t added = statistic.getAdded();
    int64_t updated = statistic.getUpdated();
    Json stat =
        JSON( StatisticFields::Hits << hits << StatisticFields::Added << added << StatisticFields::Updated << updated );
    return stat;
}

JsonObject SwingerShutdown::swingerShutdownReset( const Json& request ) {
    using namespace Resources::Settings;
    using namespace Resources::TimeSeriesFields;
    Json answer;

    std::string deviceId;
    if( request.find( Settings::DeviceId ) != request.end() ) {
        deviceId = json_get_str( request, Settings::DeviceId );
    }
    else {
        setErrorWithField( answer, Errors::WRONG_PARAMS, request.dump(), Settings::DeviceId );
        return answer;
    }

    markTimeSeriesRecordsAsDeleted( deviceId, DBTables::TempTimeSeriesDB );
    markTimeSeriesRecordsAsDeleted( deviceId, DBTables::PersistentTimeSeriesDB );

    clean();
    emitEvent( MODES_SWINGER_SHUTDOWN_RESET_DONE, JSON( Resources::Settings::DeviceId << deviceId ) );
    return answer;
}

void SwingerShutdown::emitEvent( MessageSubClasses msgClass, const Json& payload ) const {
    m_service.sendBroadcast( msgClass, payload, GroupsNames::UI_GROUP );
}

JsonObject SwingerShutdown::swingerShutdownDisableAdd( const Json& request ) {
    using namespace Resources::Settings;
    using namespace Resources::TimeSeriesFields;
    Json answer = JSON_EMPTY_OBJ;

    std::string deviceId;
    if( request.find( Settings::DeviceId ) != request.end() ) {
        deviceId = json_get_str( request, Settings::DeviceId );
    }
    else {
        setErrorWithField( answer, Errors::WRONG_PARAMS, request.dump(), Settings::DeviceId );
        return answer;
    }

    JsonObject data = SwingerShutdownSettings::loadFromDb( HouseModesSwingerShutdown, m_dbClient );
    SwingerShutdownSettings settings( data );

    if( settings.isDisabledDevice( deviceId ) ) {
        setErrorWithField( answer, Errors::ALREADY_EXISTS, request.dump(), Settings::DeviceId );
        return answer;
    }

    settings.addDisabledDevice( deviceId );
    SwingerShutdownSettings::updateDbRecord( settings.toJson(), HouseModesSwingerShutdown, m_dbClient );
    emitEvent( MODES_SWINGER_SHUTDOWN_DISABLE_ADDED, JSON( Resources::Settings::DeviceId << deviceId ) );
    return answer;
}

JsonObject SwingerShutdown::swingerShutdownDisableRemove( const Json& request ) {
    using namespace Resources::Settings;
    using namespace Resources::TimeSeriesFields;
    Json answer = JSON_EMPTY_OBJ;

    std::string deviceId;
    if( request.find( Settings::DeviceId ) != request.end() ) {
        deviceId = json_get_str( request, Settings::DeviceId );
    }
    else {
        setErrorWithField( answer, Errors::WRONG_PARAMS, request.dump(), Settings::DeviceId );
        return answer;
    }

    JsonObject data = SwingerShutdownSettings::loadFromDb( HouseModesSwingerShutdown, m_dbClient );
    SwingerShutdownSettings settings( data );
    if( !settings.isDisabledDevice( deviceId ) ) {
        setErrorWithField( answer, Errors::DOES_NOT_EXIST, request.dump(), Settings::DeviceId );
        return answer;
    }
    settings.removeDisabledDevice( deviceId );
    SwingerShutdownSettings::updateDbRecord( settings.toJson(), HouseModesSwingerShutdown, m_dbClient );
    emitEvent( MODES_SWINGER_SHUTDOWN_DISABLE_REMOVED, JSON( Resources::Settings::DeviceId << deviceId ) );

    return answer;
}

bool SwingerShutdown::isArmedFieldPresentInDevice( const std::string& deviceId ) {
    auto core = CoreDRemoteInterface::makeInstance( m_service );
    auto deviceInfo = core->getSingleDevice( deviceId );
    Json armedField;
    auto res = json_get( deviceInfo, "armed", armedField );
    return res;
}

JsonObject SwingerShutdown::swingerShutdownLimitsSet( const Json& request ) {
    using namespace Resources::Settings;
    using namespace Resources::TimeSeriesFields;
    Json answer = JSON_EMPTY_OBJ;

    std::string deviceId;
    if( request.find( Settings::DeviceId ) != request.end() ) {
        deviceId = json_get_str( request, Settings::DeviceId );
    }
    else {
        setErrorWithField( answer, Errors::WRONG_PARAMS, request.dump(), Settings::DeviceId );
        return answer;
    }

    if( !isArmedFieldPresentInDevice( deviceId ) ) {
        setError( answer, Errors::WRONG_PARAMS, "No armed device by this ID" );
        return answer;
    }

    int hitsLimit;
    if( request.find( Settings::DeviceId ) != request.end() ) {
        hitsLimit = json_get_int( request, Settings::HitsLimit );
    }
    else {
        setErrorWithField( answer, Errors::WRONG_PARAMS, request.dump(), Settings::DeviceId );
        return answer;
    }

    int inactivityWindow;
    if( request.find( Settings::DeviceId ) != request.end() ) {
        inactivityWindow = json_get_int( request, Settings::InactivityWindow );
    }
    else {
        setErrorWithField( answer, Errors::WRONG_PARAMS, request.dump(), Settings::DeviceId );
        return answer;
    }

    JsonObject data = SwingerShutdownSettings::loadFromDb( HouseModesSwingerShutdown, m_dbClient );
    SwingerShutdownSettings settings( data );

    settings.setDeviceLimits( deviceId, hitsLimit, inactivityWindow );

    SwingerShutdownSettings::updateDbRecord( settings.toJson(), HouseModesSwingerShutdown, m_dbClient );

    emitEvent( MODES_SWINGER_SHUTDOWN_LIMITS_CHANGED, JSON( Resources::Settings::DeviceId << deviceId ) );

    clean();
    return answer;
}

JsonObject SwingerShutdown::swingerShutdownLimitsReset( const Json& request ) {
    using namespace Resources::Settings;
    using namespace Resources::TimeSeriesFields;
    Json answer = JSON_EMPTY_OBJ;

    std::string deviceId;
    if( request.find( Settings::DeviceId ) != request.end() ) {
        deviceId = json_get_str( request, Settings::DeviceId );
    }
    else {
        setErrorWithField( answer, Errors::WRONG_PARAMS, request.dump(), Settings::DeviceId );
        return answer;
    }

    JsonObject data = SwingerShutdownSettings::loadFromDb( HouseModesSwingerShutdown, m_dbClient );
    SwingerShutdownSettings settings( data );
    settings.resetDeviceLimits( deviceId );
    SwingerShutdownSettings::updateDbRecord( settings.toJson(), HouseModesSwingerShutdown, m_dbClient );
    emitEvent( MODES_SWINGER_SHUTDOWN_LIMITS_CHANGED, JSON( Resources::Settings::DeviceId << deviceId ) );
    clean();
    return answer;
}

}  // namespace SwingerShutdown
}  // namespace CoreD
}  // namespace HubPlatform
