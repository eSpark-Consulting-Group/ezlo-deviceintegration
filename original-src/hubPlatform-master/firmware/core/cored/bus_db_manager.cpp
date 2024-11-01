#include "cored/bus_db_manager.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace CoreD {

BusDbManager::BusDbManager( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

JsonObject BusDbManager::handleRequestDBData( const JsonObject& params ) {
    auto response = JSON_EMPTY_OBJ;

    std::string operation = json_get_str( params, "operation" );
    if( operation == "exist" ) {
        auto dbName = json_get_str( params, "dbName" );
        response[ "exist" ] = m_dbClient.exist( dbName );
    }
    else if( operation == "size" ) {
        auto dbName = json_get_str( params, "dbName" );
        response[ "size" ] = m_dbClient.size( dbName );
    }
    else if( operation == "getPrimaryKeys" ) {
        auto dbName = json_get_str( params, "dbName" );
        auto primaryKeys = m_dbClient.getPrimaryKeys( dbName );
        response[ "primaryKeys" ] = primaryKeys;
    }
    else if( operation == "getAllCollectionsNames" ) {
        auto onlyWithChanges = json_get_bool( params, "onlyWithChanges" );
        auto allCollectionsNames = m_dbClient.getAllCollectionsNames( onlyWithChanges );
        response[ "allCollectionsNames" ] = allCollectionsNames;
    }
    else if( operation == "getRecords" ) {
        auto dbName = json_get_str( params, "dbName" );
        auto query = params[ "query" ];
        auto n = json_get_int( params, "n" );
        auto nToSkip = json_get_int( params, "nToSkip" );

        auto records = JSON_EMPTY_ARR;
        m_dbClient.forEachNth(
            dbName, query, [&]( Json data, const size_t ) { records.push_back( std::move( data ) ); }, n, nToSkip );

        response[ "records" ] = records;
    }
    else {
        LOGGER_ERROR_RAW( cc::error( "Got unexpected db request: " ) + cc::note( operation ) );
    }

    return response;
}

void BusDbManager::handlePerformDBOperation( const JsonObject& params ) {
    std::string operation = json_get_str( params, "operation" );
    if( operation == "insert" ) {
        auto dbName = json_get_str( params, "dbName" );
        auto obj = params[ "obj" ];
        auto lockIgnore = json_get_bool( params, "lockIgnore" );
        m_dbClient.insert( dbName, obj, lockIgnore );
    }
    else if( operation == "update" ) {
        auto dbName = json_get_str( params, "dbName" );
        auto query = params[ "query" ];
        auto obj = params[ "obj" ];
        auto upsert = json_get_bool( params, "upsert" );
        auto multi = json_get_bool( params, "multi" );
        m_dbClient.update( dbName, query, obj, upsert, multi );
    }
    else if( operation == "remove" ) {
        auto dbName = json_get_str( params, "dbName" );
        auto query = params[ "query" ];
        auto justOne = json_get_bool( params, "justOne" );
        auto lockIgnore = json_get_bool( params, "lockIgnore" );
        m_dbClient.remove( dbName, query, justOne, lockIgnore );
    }
    else if( operation == "capped" ) {
        auto dbName = json_get_str( params, "dbName" );
        auto maxCollectionSize = json_get_int( params, "maxCollectionSize" );
        m_dbClient.capped( dbName, maxCollectionSize );
    }
    else if( operation == "create" ) {
        auto dbName = json_get_str( params, "dbName" );
        m_dbClient.create( dbName );
    }
    else if( operation == "clear" ) {
        auto dbName = json_get_str( params, "dbName" );
        m_dbClient.clear( dbName );
    }
    else if( operation == "drop" ) {
        auto dbName = json_get_str( params, "dbName" );
        m_dbClient.drop( dbName );
    }
    else if( operation == "clean" ) {
        m_dbClient.clean();
    }
    else if( operation == "fsync" ) {
        m_dbClient.fsync();
    }
    else if( operation == "reset" ) {
        m_dbClient.reset();
    }
    else if( operation == "lock" ) {
        m_dbClient.lock();
    }
    else if( operation == "unlock" ) {
        m_dbClient.unlock();
    }
    else if( operation == "resetChangesCounter" ) {
        m_dbClient.resetChangesCounter();
    }
    else if( operation == "fixBrokenPrimaryKeys" ) {
        auto dbName = json_get_str( params, "dbName" );
        m_dbClient.fixBrokenPrimaryKeys( dbName );
    }
    else {
        LOGGER_ERROR_RAW( cc::error( "Got unexpected db operation: " ) + cc::note( operation ) );
    }
}

}  // namespace CoreD
}  // namespace HubPlatform
