#include "swinger_shutdown_time_series.h"
#include "database_client/dbid.h"
#include "database_client/dbqueryparser.h"
#include "database_client/idbclient.h"
#include "resources.h"

namespace HubPlatform {
namespace CoreD {
namespace SwingerShutdown {

using namespace Resources::TimeSeriesFields;

SwingerShutdownTimeSeries::SwingerShutdownTimeSeries( Json& record ) {
    fromJson( record );
}

std::string SwingerShutdownTimeSeries::getId() const {
    return m_id;
}

std::string SwingerShutdownTimeSeries::getDeviceId() const {
    return m_deviceId;
}

int SwingerShutdownTimeSeries::getHits() const {
    return m_hits;
}

int64_t SwingerShutdownTimeSeries::getBegin() const {
    return m_begin;
}

int64_t SwingerShutdownTimeSeries::getUpdated() const {
    return m_updated;
}

bool SwingerShutdownTimeSeries::getDeleted() const {
    return m_deleted;
}

void SwingerShutdownTimeSeries::setId( const std::string& id ) {
    m_id = id;
}

void SwingerShutdownTimeSeries::setDeviceId( const std::string& id ) {
    m_deviceId = id;
}

void SwingerShutdownTimeSeries::setHits( int hits ) {
    m_hits = hits;
}

void SwingerShutdownTimeSeries::setBegin( int64_t begin ) {
    m_begin = begin;
}

void SwingerShutdownTimeSeries::setUpdated( int64_t updated ) {
    m_updated = updated;
}

void SwingerShutdownTimeSeries::setDeleted( bool deleted ) {
    m_deleted = deleted;
}

Json SwingerShutdownTimeSeries::toJson() const {
    JsonObject result = JSON_EMPTY_OBJ;
    result[ Id ] = m_id;
    result[ DeviceId ] = m_deviceId;
    result[ Hits ] = m_hits;
    result[ Begin ] = m_begin;
    result[ Updated ] = m_updated;
    result[ Deleted ] = m_deleted;
    return result;
}

bool SwingerShutdownTimeSeries::fromJson( const JsonObject& data ) {
    try {
        auto param = data.find( Id );
        if( param != data.end() && param->is_string() ) {
            m_id = json_get_str( data, Id );
        }

        param = data.find( DeviceId );
        if( param != data.end() && param->is_string() ) {
            m_deviceId = json_get_str( data, DeviceId );
        }

        param = data.find( Hits );
        if( param != data.end() && param->is_number() ) {
            m_hits = json_get_int( data, Hits );
        }

        param = data.find( Begin );
        if( param != data.end() && param->is_number() ) {
            m_begin = json_get_int64( data, Begin );
        }

        param = data.find( Updated );
        if( param != data.end() && param->is_number() ) {
            m_updated = json_get_int64( data, Updated );
        }

        param = data.find( Deleted );
        if( param != data.end() && param->is_boolean() ) {
            m_deleted = json_get_bool( data, Deleted );
        }
        return true;
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Failed to extract time series: " ) + cc::note( ex.what() ) );
    }

    return false;
}

bool SwingerShutdownTimeSeries::isSupportedDBName( const std::string& dbName ) {
    bool isSupportedTableName =
        dbName == Resources::DBTables::TempTimeSeriesDB || dbName == Resources::DBTables::PersistentTimeSeriesDB;

    if( !isSupportedTableName ) {
        LOGGER_ERROR( "Not supported DB name fo swinger shutdown time series operations: " + dbName );
        return false;
    }

    return true;
}

bool SwingerShutdownTimeSeries::createDbRecord( std::string id,
                                                JsonObject query,
                                                const std::string& dbName,
                                                IDbClient& dbClient ) {
    if( !isSupportedDBName( dbName ) )
        return false;

    if( query.empty() ) {
        query[ db::ID_FIELD_NAME ] = id;
        query[ DeviceId ] = "";
        query[ Hits ] = 0;
        query[ Begin ] = 0;
        query[ Updated ] = 0;
        query[ Deleted ] = false;
    }
    dbClient.insert( dbName, query, true );
    return true;
}

bool SwingerShutdownTimeSeries::updateDbRecord( std::string id,
                                                JsonObject updateObj,
                                                const std::string& dbName,
                                                IDbClient& dbClient ) {
    if( !isSupportedDBName( dbName ) ) {
        return false;
    }

    updateObj[ db::ID_FIELD_NAME ] = id;
    dbClient.update( dbName, JSON( Id << id ), updateObj, false, true );
    return true;
}

Json SwingerShutdownTimeSeries::loadFromDb( std::string id, const std::string& dbName, IDbClient& dbClient ) {
    if( !isSupportedDBName( dbName ) )
        return JSON_NULL_VAL;

    auto records = dbClient.findOne( dbName, JSON( Id << id ), nullptr );
    return records;
}

JsonArray SwingerShutdownTimeSeries::loadRecordsFromDbWithDeviceId( std::string id,
                                                                    const std::string& dbName,
                                                                    IDbClient& dbClient ) {
    auto records = dbClient.findAll( dbName, JSON( DeviceId << id ), nullptr );
    return records;
}

bool SwingerShutdownTimeSeries::deleteDbRecord( std::string id, const std::string& dbName, IDbClient& dbClient ) {
    if( !isSupportedDBName( dbName ) )
        return false;

    dbClient.remove( dbName, JSON( Id << id ) );
    return true;
}

JsonArray SwingerShutdownTimeSeries::loadAllRecordsFromDb( const std::string& dbName,
                                                           const JsonObject& object,
                                                           IDbClient& dbClient ) {
    if( !isSupportedDBName( dbName ) )
        return JSON_NULL_VAL;

    auto allRecords = dbClient.findAll( dbName, object, nullptr );
    return allRecords;
}

}  // namespace SwingerShutdown
}  // namespace CoreD
}  // namespace HubPlatform
