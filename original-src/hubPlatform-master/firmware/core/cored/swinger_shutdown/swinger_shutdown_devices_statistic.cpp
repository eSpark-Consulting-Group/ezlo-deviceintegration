#include "swinger_shutdown_devices_statistic.h"
#include "database_client/dbid.h"
#include "database_client/dbqueryparser.h"
#include "database_client/idbclient.h"
#include "resources.h"

namespace HubPlatform {
namespace CoreD {
namespace SwingerShutdown {

using namespace Resources::StatisticFields;

bool SwingerShutdownDeviceStatistic::isSupportedDBName( const std::string& dbName ) {
    bool isSupportedTableName =
        dbName == Resources::DBTables::TempStatisticDB || dbName == Resources::DBTables::PersistentStatisticDB;

    if( !isSupportedTableName ) {
        LOGGER_ERROR( "Not supported DB name for swinger shutdown device statistic operations: " + dbName );
        return false;
    }

    return true;
}

SwingerShutdownDeviceStatistic::SwingerShutdownDeviceStatistic( const Json& record ) {
    fromJson( record );
}

std::string SwingerShutdownDeviceStatistic::getId() const {
    return m_id;
}

int SwingerShutdownDeviceStatistic::getHits() const {
    return m_hits;
}

int64_t SwingerShutdownDeviceStatistic::getAdded() const {
    return m_added;
}

int64_t SwingerShutdownDeviceStatistic::getUpdated() const {
    return m_updated;
}

bool SwingerShutdownDeviceStatistic::getDeleted() const {
    return m_deleted;
}

void SwingerShutdownDeviceStatistic::setId( const std::string& id ) {
    m_id = id;
}

void SwingerShutdownDeviceStatistic::setHits( int hits ) {
    m_hits = hits;
}

void SwingerShutdownDeviceStatistic::setAdded( int64_t added ) {
    m_added = added;
}

void SwingerShutdownDeviceStatistic::setUpdated( int64_t updated ) {
    m_updated = updated;
}

void SwingerShutdownDeviceStatistic::setDeleted( bool deleted ) {
    m_deleted = deleted;
}

Json SwingerShutdownDeviceStatistic::toJson() const {
    JsonObject result = JSON_EMPTY_OBJ;
    result[ Id ] = m_id;
    result[ Hits ] = m_hits;
    result[ Added ] = m_added;
    result[ Updated ] = m_updated;
    result[ Deleted ] = m_deleted;
    return result;
}

bool SwingerShutdownDeviceStatistic::fromJson( const JsonObject& data ) {
    try {
        auto param = data.find( Id );
        if( param != data.end() && param->is_string() ) {
            m_id = json_get_str( data, Id );
        }

        param = data.find( Hits );
        if( param != data.end() && param->is_number() ) {
            m_hits = json_get_int( data, Hits );
        }

        param = data.find( Added );
        if( param != data.end() && param->is_number() ) {
            m_added = json_get_int64( data, Added );
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
        LOGGER_ERROR( cc::error( "Failed to extract device statistic: " ) + cc::note( ex.what() ) );
    }

    return false;
}

bool SwingerShutdownDeviceStatistic::createDbRecord( const std::string& id,
                                                     JsonObject query,
                                                     const std::string& dbName,
                                                     IDbClient& dbClient ) {
    if( !isSupportedDBName( dbName ) )
        return false;

    if( query.empty() ) {
        query[ db::ID_FIELD_NAME ] = id;
        query[ Hits ] = 0;
        query[ Added ] = 0;
        query[ Updated ] = 0;
        query[ Deleted ] = false;
    }
    dbClient.insert( dbName, query, true );

    return true;
}

bool SwingerShutdownDeviceStatistic::updateDbRecord( const std::string& id,
                                                     JsonObject updateObj,
                                                     const std::string& dbName,
                                                     IDbClient& dbClient ) {
    if( !isSupportedDBName( dbName ) )
        return false;

    using namespace Resources::StatisticFields;
    updateObj[ db::ID_FIELD_NAME ] = id;
    dbClient.update( dbName, JSON( Id << id ), updateObj, false, true );

    return true;
}

Json SwingerShutdownDeviceStatistic::loadFromDb( const std::string& id,
                                                 const std::string& dbName,
                                                 IDbClient& dbClient ) {
    if( !isSupportedDBName( dbName ) )
        return JSON_NULL_VAL;

    auto records = dbClient.findOne( dbName, JSON( Id << id ), nullptr );
    return records;
}

bool SwingerShutdownDeviceStatistic::deleteDbRecord( const std::string& id,
                                                     const std::string& dbName,
                                                     IDbClient& dbClient ) {
    if( !isSupportedDBName( dbName ) )
        return false;

    dbClient.remove( dbName, JSON( Id << id ) );
    return true;
}

JsonArray SwingerShutdownDeviceStatistic::loadAllFromDb( const std::string& dbName,
                                                         JsonObject& request,
                                                         const Json* fieldsToReturn,
                                                         IDbClient& dbClient ) {
    if( !isSupportedDBName( dbName ) )
        return JSON_NULL_VAL;

    return dbClient.findAll( dbName, request, fieldsToReturn );
}

}  // namespace SwingerShutdown
}  // namespace CoreD
}  // namespace HubPlatform
